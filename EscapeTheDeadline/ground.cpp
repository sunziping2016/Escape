#include <stdio.h>
#include "ground.h"
#include "drawer.h"
#include "loader.h"
#include "engine.h"
#include "collision.h"
#include "world.h"
#include "timer.h"
#include "player.h"

#define MAX_BORDERLEN	10
#define MAX_GROUNDLEN	1000

#define WIDTH_BORDER	6.0
#define GROUND_WIDTH	10.0
#define COLOR_BORDER	RGB(0xff, 0x00, 0x00)

double groundFriction;
double groundGravity;
COLORREF backgroundColor;

static HBRUSH hBrushBackground;
static HPEN hPenBorder, hPenNoOutline;

static struct
{
	double fromX, fromY;
	double toX, toY;
	int visible;
} borders[MAX_BORDERLEN];
static int bordersEnd;

static int BorderCreate(wchar_t *command)
{
	if (bordersEnd == MAX_BORDERLEN) return 1;
	if (swscanf(command, L"%*s%lf%lf%lf%lf%d", &borders[bordersEnd].fromX, &borders[bordersEnd].fromY,
			&borders[bordersEnd].toX, &borders[bordersEnd].toY, &borders[bordersEnd].visible) == 5 &&
			(borders[bordersEnd].fromX != borders[bordersEnd].toX || borders[bordersEnd].fromY != borders[bordersEnd].toY)) {
		++bordersEnd;
		return 0;
	}
	return 1;
}
static int FrictionCreate(wchar_t *command)
{
	if (swscanf(command, L"%*s%lf", &groundFriction) == 1)
		return 0;
	return 1;
}
static int GravityCreate(wchar_t *command)
{
	if (swscanf(command, L"%*s%lf", &groundGravity) == 1)
		return 0;
	return 1;
}
static int BackgroundCreate(wchar_t *command)
{
	if (swscanf(command, L"%*s%lx", &backgroundColor) == 1)
		backgroundColor = DrawerRGB(backgroundColor);
		return 0;
	return 1;
}

static void BorderDrawer(int id, HDC hDC)
{
	if (gameState != STARTED || !borders[id].visible) return;
	WorldSetViewport(0.0, 0.0);
	SelectObject(hDC, hPenBorder);
	MoveToEx(hDC, WorldX(borders[id].fromX), WorldY(borders[id].fromY), NULL);
	LineTo(hDC, WorldX(borders[id].toX), WorldY(borders[id].toY));
}
static Points *BorderCollision(int id)
{
	static Points points;
	points.points[0][0] = borders[id].fromX;	points.points[0][1] = borders[id].fromY;
	points.points[1][0] = borders[id].toX;		points.points[1][1] = borders[id].toY;
	points.n = 2;
	return &points;
}
static Types borderCollisionTypes = { { ID_PLAYER }, 1 };
static void BorderCollisionNotifier(int id, int othertype, int otherid, double n[2], double depth)
{
	if (othertype == ID_PLAYER)
		PlayerDie();
}
static void BackgroundDrawer(int id, HDC hDC)
{
	RECT rect;
	if (gameState != STARTED) return;
	rect.left = 0;			rect.top = 0;
	rect.right = DrawerX;	rect.bottom = DrawerY;
	FillRect(hDC, &rect, hBrushBackground);
}

// How to change visible state after collision
#define SHOWNORMAL				0
#define SHOWALWAYS				1  // change to SHOWNORMAL after collision
#define SHOWONCE				2

#define GROUND_NORMAL			0
#define GROUND_DIE				1
#define GROUND_NOJUMP			2
#define GROUND_NOMOVE			3
#define GROUND_NOCONTROL		4
static COLORREF groundColor[] = {
	RGB(0x80, 0x80, 0x80),		// Normal
	RGB(0xff, 0x00, 0x00),		// Die
	RGB(0x00, 0x80, 0x00),		// No jump
	RGB(0x70, 0x80, 0x90),		// No move
	RGB(0x00, 0xbf, 0xff)		// No control
};

static struct {
	double fromX, fromY;
	double toX, toY;
	double color;
	int traits;
	int invisible;
} grounds[MAX_GROUNDLEN];
static int groundsEnd;

static void AngularBisector(double vx1, double vy1, double vx2, double vy2, double *vx, double *vy)
{
	double temp;
	temp = sqrt(vx1 * vx1 + vy1 * vy1);
	vx1 /= temp;		vy1 /= temp;
	temp = sqrt(vx2 * vx2 + vy2 * vy2);
	vx2 /= temp;		vy2 /= temp;
	*vx = vx1 + vx2;	*vy = vy1 + vy2;
	if (*vx * *vx + *vy * *vy == 0.0) {
		*vx = vy1;
		*vy = -vx1;
	}
	else {
		temp = sqrt(*vx * *vx + *vy * *vy);
		*vx /= temp;
		*vy /= temp;
	}
}

static void GroundDrawer(int id, HDC hDC)
{
	double leftN[2], rightN[2], groundN[2], len;
	POINT points[4];
	COLORREF color;
	int drawLeft = 0, drawRight = 0;
	HBRUSH hBrush;
	HPEN hPen;
	if (gameState != STARTED) return;
	if (grounds[id].invisible || viewRect[2] < grounds[id].fromX || viewRect[0] > grounds[id].toX ||
		(grounds[id].fromY > viewRect[3] && grounds[id].toY > viewRect[3])) return;
	color = DrawerColor(groundColor[grounds[id].traits], backgroundColor, grounds[id].color);
	hPen = CreatePen(PS_SOLID, 3, color);
	hBrush = CreateSolidBrush(DrawerColor(color, backgroundColor, 0.4));
	SelectObject(hDC, hBrush);
	groundN[0] = grounds[id].fromY - grounds[id].toY;
	groundN[1] = grounds[id].toX - grounds[id].fromX;
	if (id != 0 && !grounds[id - 1].invisible && grounds[id - 1].toX == grounds[id].fromX && grounds[id - 1].toY == grounds[id].fromY) {
		AngularBisector(grounds[id - 1].fromX - grounds[id - 1].toX, grounds[id - 1].fromY - grounds[id - 1].toY,
			grounds[id].toX - grounds[id].fromX, grounds[id].toY - grounds[id].fromY, &leftN[0], &leftN[1]);
	}
	else {
		AngularBisector(0.0, 1.0, grounds[id].toX - grounds[id].fromX, grounds[id].toY - grounds[id].fromY, &leftN[0], &leftN[1]);
		drawLeft = 1;
	}
	len = (groundN[0] * leftN[0] + groundN[1] * leftN[1]) / sqrt(groundN[0] * groundN[0] + groundN[1] * groundN[1]);
	leftN[0] *= GROUND_WIDTH / len;
	leftN[1] *= GROUND_WIDTH / len;
	if (drawLeft) {
		WorldSetViewport(grounds[id].fromX, grounds[id].fromY);
		points[0].x = WorldX(0);			points[0].y = WorldY(0);
		points[1].x = WorldX(leftN[0]);		points[1].y = WorldY(leftN[1]);
		points[2].x = WorldX(GROUND_WIDTH);	points[2].y = DrawerY;
		points[3].x = WorldX(0);			points[3].y = DrawerY;
		SelectObject(hDC, hPenNoOutline);
		Polygon(hDC, points, 4);
		SelectObject(hDC, hPen);
		MoveToEx(hDC, points[0].x, points[0].y, NULL);
		LineTo(hDC, points[3].x, points[3].y);
	}
	if (id != groundsEnd - 1 && !grounds[id + 1].invisible && grounds[id + 1].fromX == grounds[id].toX && grounds[id + 1].fromY == grounds[id].toY) {
		AngularBisector(grounds[id].fromX - grounds[id].toX, grounds[id].fromY - grounds[id].toY,
			grounds[id + 1].toX - grounds[id + 1].fromX, grounds[id + 1].toY - grounds[id + 1].fromY, &rightN[0], &rightN[1]);
	}
	else {
		AngularBisector(grounds[id].fromX - grounds[id].toX, grounds[id].fromY - grounds[id].toY, 0.0, 1.0, &rightN[0], &rightN[1]);
		drawRight = 1;
	}
	len = (groundN[0] * rightN[0] + groundN[1] * rightN[1]) / sqrt(groundN[0] * groundN[0] + groundN[1] * groundN[1]);
	rightN[0] *= GROUND_WIDTH / len;
	rightN[1] *= GROUND_WIDTH / len;
	if (drawRight) {
		WorldSetViewport(grounds[id].toX, grounds[id].toY);
		points[0].x = WorldX(0);				points[0].y = WorldY(0);
		points[1].x = WorldX(rightN[0]);		points[1].y = WorldY(rightN[1]);
		points[2].x = WorldX(-GROUND_WIDTH);	points[2].y = DrawerY;
		points[3].x = WorldX(0);				points[3].y = DrawerY;
		SelectObject(hDC, hPenNoOutline);
		Polygon(hDC, points, 4);
		SelectObject(hDC, hPen);
		MoveToEx(hDC, points[0].x, points[0].y, NULL);
		LineTo(hDC, points[3].x, points[3].y);
	}
	WorldSetViewport(0.0, 0.0);
	points[0].x = WorldX(grounds[id].fromX);			points[0].y = WorldY(grounds[id].fromY);
	points[1].x = WorldX(grounds[id].toX);				points[1].y = WorldY(grounds[id].toY);
	points[2].x = WorldX(grounds[id].toX + rightN[0]);	points[2].y = WorldY(grounds[id].toY + rightN[1]);
	points[3].x = WorldX(grounds[id].fromX + leftN[0]);	points[3].y = WorldY(grounds[id].fromY + leftN[1]);
	SelectObject(hDC, hPenNoOutline);
	Polygon(hDC, points, 4);
	SelectObject(hDC, hPen);
	MoveToEx(hDC, points[0].x, points[0].y, NULL);
	LineTo(hDC, points[1].x, points[1].y);
	DeleteObject(hBrush);
	DeleteObject(hPen);
}
static Points *GroundCollision(int id)
{
	static Points points;
	points.points[0][0] = grounds[id].fromX;	points.points[0][1] = grounds[id].fromY + 0.5;
	points.points[1][0] = grounds[id].fromX;	points.points[1][1] = grounds[id].fromY;
	points.points[2][0] = grounds[id].toX;		points.points[2][1] = grounds[id].toY;
	points.points[3][0] = grounds[id].toX;		points.points[3][1] = grounds[id].toY + 0.5;
	points.n = 4;
	return &points;
}
#define STEP_GROUNDVISIBLE		0.05
static void GroundVisibleTimer(int id, int ms)
{
	if (gameState != STARTED) return;
	grounds[id].color += STEP_GROUNDVISIBLE;
	if (grounds[id].color >= 1.0) {
		grounds[id].color = 1.0;
	}
	else
		TimerAdd(GroundVisibleTimer, id, ms + 20);
}
static Types groundCollisionTypes = { { ID_PLAYER }, 1 };
static void GroundCollisionNotifier(int id, int othertype, int otherid, double n[2], double depth)
{
	if (othertype == ID_PLAYER) {
		if (grounds[id].invisible) {
			grounds[id].invisible = 0;
			TimerAdd(GroundVisibleTimer, id, 20);
		}
		switch (grounds[id].traits)
		{
		case GROUND_DIE:
			PlayerMinusLife(0.4 * PLAYERLIFE_MAX);
			break;
		default:
			break;
		}
	}
}

int GroundCreate(wchar_t *command)
{
	if (groundsEnd == MAX_GROUNDLEN) return 1;
	if (swscanf(command, L"%*s%lf%lf%lf%lf%d%d", &grounds[groundsEnd].fromX, &grounds[groundsEnd].fromY,
		&grounds[groundsEnd].toX, &grounds[groundsEnd].toY, &grounds[groundsEnd].traits, &grounds[groundsEnd].invisible) == 6 &&
		grounds[groundsEnd].toX >= grounds[groundsEnd].fromX) {
		++groundsEnd;
		return 0;
	}
	return 1;
}

void GroundInit()
{
	backgroundColor = RGB(0xff, 0xff, 0xff);
	hPenBorder = CreatePen(PS_SOLID, 1, COLOR_BORDER);
	hPenNoOutline = CreatePen(PS_NULL, 0, 0);
	DrawerAdd(BackgroundDrawer, 0, 11);
	LoaderAdd(L"border", BorderCreate);
	LoaderAdd(L"gravity", GravityCreate);
	LoaderAdd(L"friction", FrictionCreate);
	LoaderAdd(L"background", BackgroundCreate);
	LoaderAdd(L"ground", GroundCreate);
}
void GroundDestroy()
{
	DeleteObject(hPenNoOutline);
	DeleteObject(hPenBorder);
}
void GroundStart()
{
	int i;
	for (i = 0; i < bordersEnd; ++i) {
		if (borders[i].visible)
			DrawerAdd(BorderDrawer, i, 0);
		CollisionAdd(BorderCollision, i, ID_BORDER, &borderCollisionTypes, BorderCollisionNotifier);
	}
	for (i = 0; i < groundsEnd; ++i) {
		if (!grounds[i].invisible)
			grounds[i].color = 1.0;
		else
			grounds[i].color = 0.0;
		DrawerAdd(GroundDrawer, i, 6 - grounds[i].traits);
		if (grounds[i].fromX != grounds[i].toX)
			CollisionAdd(GroundCollision, i, ID_GROUND, &groundCollisionTypes, GroundCollisionNotifier);
	}
	hBrushBackground = CreateSolidBrush(backgroundColor);
}
void GroundStop()
{
	int i;
	DeleteObject(hBrushBackground);
	for (i = 0; i < groundsEnd; ++i) {
		DrawerRemove(GroundDrawer, i);
		CollisionRemove(GroundCollision, i);
	}
	for (i = 0; i < bordersEnd; ++i) {
		if (borders[i].visible)
			DrawerRemove(BorderDrawer, i);
		CollisionRemove(BorderCollision, i);
	}
	bordersEnd = groundsEnd = 0;
	groundFriction = 0.0;
	groundGravity = 0.0;
	backgroundColor = RGB(0xff, 0xff, 0xff);
}
void GroundPause() {}
void GroundResume() {}
