#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ground.h"
#include "drawer.h"
#include "loader.h"
#include "engine.h"
#include "collision.h"
#include "world.h"
#include "timer.h"
#include "player.h"
#include "commonui.h"

#define MAX_BORDERLEN	10

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
	if (swscanf(command, L"%*s%lx", &backgroundColor) == 1) {
		DeleteObject(hBrushBackground);
		backgroundColor = DrawerRGB(backgroundColor);
		hBrushBackground = CreateSolidBrush(backgroundColor);
		return 0;
	}
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
static CollisionState *BorderCollision(int id, int othertype, int otherid)
{
	static CollisionState points;
	points.points[0][0] = borders[id].fromX;	points.points[0][1] = borders[id].fromY;
	points.points[1][0] = borders[id].toX;		points.points[1][1] = borders[id].toY;
	points.velocity[0] = 0.0;					points.velocity[1] = 0.0;
	points.n = 2;
	points.usev = 0;
	return &points;
}
static CollisionType borderCollisionTypes = { { ID_PLAYER }, 1 };
static void BorderCollisionNotifier(int id, int othertype, int otherid, double n[2], double depth, int usev)
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
GroundType grounds[MAX_GROUNDLEN];
int groundsEnd;
int groundsMostId[2];
int groundsLowestID;


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
	double leftN[2], rightN[2], groundN[2], len, leftEx[2], rightEx[2], lenLeft, lenRight;
	POINT points[4];
	COLORREF color;
	int drawLeft = 0, drawRight = 0, leftId, rightId;
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
	leftId = grounds[id].leftId;
	if (leftId != -1 && !grounds[leftId].invisible && grounds[leftId].toY == grounds[id].fromY) {
		AngularBisector(grounds[leftId].fromX - grounds[leftId].toX, grounds[leftId].fromY - grounds[leftId].toY,
			grounds[id].toX - grounds[id].fromX, grounds[id].toY - grounds[id].fromY, &leftN[0], &leftN[1]);
	}
	else if (leftId != -1 && !grounds[leftId].invisible) {
		if (grounds[leftId].toY > grounds[id].fromY) {
			AngularBisector(0.0, 1.0, grounds[id].toX - grounds[id].fromX, grounds[id].toY - grounds[id].fromY, &leftN[0], &leftN[1]);
			AngularBisector(grounds[leftId].fromX - grounds[leftId].toX, grounds[leftId].fromY - grounds[leftId].toY, 0.0, -1.0, &leftEx[0], &leftEx[1]);
			leftEx[1] *= GROUND_WIDTH / leftEx[0];
			leftEx[0] *= GROUND_WIDTH / leftEx[0];
			lenLeft = grounds[leftId].toY - grounds[id].fromY;
			drawLeft = 1;
		}
		else
			AngularBisector(0.0, -1.0, grounds[id].toX - grounds[id].fromX, grounds[id].toY - grounds[id].fromY, &leftN[0], &leftN[1]);
	}
	else {
		AngularBisector(0.0, 1.0, grounds[id].toX - grounds[id].fromX, grounds[id].toY - grounds[id].fromY, &leftN[0], &leftN[1]);
		leftEx[0] = GROUND_WIDTH;
		leftEx[1] = 0.0;
		lenLeft = viewRect[3] - grounds[id].fromY;
		drawLeft = 1;
	}
	len = (groundN[0] * leftN[0] + groundN[1] * leftN[1]) / sqrt(groundN[0] * groundN[0] + groundN[1] * groundN[1]);
	leftN[0] *= GROUND_WIDTH / len;
	leftN[1] *= GROUND_WIDTH / len;
	if (drawLeft) {
		WorldSetViewport(grounds[id].fromX, grounds[id].fromY);
		points[0].x = WorldX(0);			points[0].y = WorldY(0);
		points[1].x = WorldX(leftN[0]);		points[1].y = WorldY(leftN[1]);
		points[2].x = WorldX(leftEx[0]);	points[2].y = WorldY(lenLeft + leftEx[1]);
		points[3].x = WorldX(0);			points[3].y = WorldY(lenLeft);
		SelectObject(hDC, hPenNoOutline);
		Polygon(hDC, points, 4);
		SelectObject(hDC, hPen);
		MoveToEx(hDC, points[0].x, points[0].y, NULL);
		LineTo(hDC, points[3].x, points[3].y);
	}
	rightId = grounds[id].rightId;
	if (rightId != -1 && !grounds[rightId].invisible && grounds[rightId].fromY == grounds[id].toY) {
		AngularBisector(grounds[id].fromX - grounds[id].toX, grounds[id].fromY - grounds[id].toY,
			grounds[rightId].toX - grounds[rightId].fromX, grounds[rightId].toY - grounds[rightId].fromY, &rightN[0], &rightN[1]);
	}
	else if (rightId != -1 && !grounds[rightId].invisible) {
		if (grounds[rightId].fromY > grounds[id].toY) {
			AngularBisector(grounds[id].fromX - grounds[id].toX, grounds[id].fromY - grounds[id].toY, 0.0, 1.0, &rightN[0], &rightN[1]);
			AngularBisector(0.0, -1.0, grounds[rightId].toX - grounds[rightId].fromX, grounds[rightId].toY - grounds[rightId].fromY, &rightEx[0], &rightEx[1]);
			rightEx[1] *= -GROUND_WIDTH / rightEx[0];
			rightEx[0] *= -GROUND_WIDTH / rightEx[0];
			lenRight = grounds[rightId].fromY - grounds[id].toY;
			drawRight = 1;
		}
		else
			AngularBisector(grounds[id].fromX - grounds[id].toX, grounds[id].fromY - grounds[id].toY, 0.0, -1.0, &rightN[0], &rightN[1]);
	}
	else {
		AngularBisector(grounds[id].fromX - grounds[id].toX, grounds[id].fromY - grounds[id].toY, 0.0, 1.0, &rightN[0], &rightN[1]);
		rightEx[0] = -GROUND_WIDTH;
		rightEx[1] = 0.0;
		lenRight = viewRect[3] - grounds[id].toY;
		drawRight = 1;
	}
	len = (groundN[0] * rightN[0] + groundN[1] * rightN[1]) / sqrt(groundN[0] * groundN[0] + groundN[1] * groundN[1]);
	rightN[0] *= GROUND_WIDTH / len;
	rightN[1] *= GROUND_WIDTH / len;
	if (drawRight) {
		WorldSetViewport(grounds[id].toX, grounds[id].toY);
		points[0].x = WorldX(0);				points[0].y = WorldY(0);
		points[1].x = WorldX(rightN[0]);		points[1].y = WorldY(rightN[1]);
		points[2].x = WorldX(rightEx[0]);		points[2].y = WorldY(lenRight + rightEx[1]);
		points[3].x = WorldX(0);				points[3].y = WorldY(lenRight);
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
static CollisionState *GroundCollision(int id, int othertype, int otherid)
{
	return &grounds[id].points;
}
#define STEP_GROUNDVISIBLE		0.05
static void GroundVisibleTimer(int id, int ms)
{
	if (gameState != STARTED || id < 0) return;
	grounds[id].color += STEP_GROUNDVISIBLE;
	if (grounds[id].color >= 1.0) {
		grounds[id].color = 1.0;
		grounds[id].hasTimer = 0;
	}
	else
		TimerAdd(GroundVisibleTimer, id, ms + 20);
}
static CollisionType groundCollisionTypes = { { ID_PLAYER }, 1 };
static void GroundCollisionNotifier(int id, int othertype, int otherid, double n[2], double depth, int usev)
{
	if (othertype == ID_PLAYER) {
		if (grounds[id].invisible && grounds[id].hasTimer == 0) {
			grounds[id].invisible = 0;
			grounds[id].hasTimer = 1;
			TimerAdd(GroundVisibleTimer, id, 20);
		}
		switch (grounds[id].traits)
		{
		case GROUND_DIE:
			PlayerMinusLife(0.2 * PLAYERLIFE_MAX);
			break;
		default:
			break;
		}
	}
}
int GroundAdd(double fromX, double fromY, double toX, double toY, int traits, int invisible)
{
	int i = 0;
	double lower;
	if (groundsEnd == MAX_GROUNDLEN) {
		ErrorPrintf(L"GroundError: Resource used up.");
		return -1;
	}
	grounds[groundsEnd].leftId = grounds[groundsEnd].rightId = -1;
	for (i = 0; i < groundsEnd; ++i) {
		if (grounds[i].fromX < toX && fromX < grounds[i].toX)
			return 1;
		else if (grounds[i].fromX == toX) {
			grounds[i].leftId = groundsEnd;
			grounds[groundsEnd].rightId = i;
		}
		else if (grounds[i].toX == fromX) {
			grounds[i].rightId = groundsEnd;
			grounds[groundsEnd].leftId = i;
		}
	}
	grounds[groundsEnd].fromX = fromX;
	grounds[groundsEnd].fromY = fromY;
	grounds[groundsEnd].toX = toX;
	grounds[groundsEnd].toY = toY;
	grounds[groundsEnd].traits = traits;
	grounds[groundsEnd].invisible = invisible;
	if (groundsMostId[0] == -1 || fromX < grounds[groundsMostId[0]].fromX)
		groundsMostId[0] = groundsEnd;
	if (groundsMostId[1] == -1 || toX > grounds[groundsMostId[1]].toX)
		groundsMostId[1] = groundsEnd;
	if (fromY > toY) lower = fromY;
	else lower = toY;
	if (groundsLowestID == -1 || grounds[groundsLowestID].fromY < lower && grounds[groundsLowestID].toY < lower)
		groundsLowestID = groundsEnd;
	grounds[groundsEnd].color = 0.0;
	grounds[groundsEnd].hasTimer = 0;
	if (!grounds[groundsEnd].invisible && gameState == STARTED) {
		grounds[groundsEnd].hasTimer = 1;
		TimerAdd(GroundVisibleTimer, groundsEnd, 20);
	}
	else if(!grounds[groundsEnd].invisible)
		grounds[groundsEnd].color = 1.0;
	grounds[groundsEnd].points.points[0][0] = fromX;	grounds[groundsEnd].points.points[0][1] = fromY + 0.5;
	grounds[groundsEnd].points.points[1][0] = fromX;	grounds[groundsEnd].points.points[1][1] = fromY;
	grounds[groundsEnd].points.points[2][0] = toX;		grounds[groundsEnd].points.points[2][1] = toY;
	grounds[groundsEnd].points.points[3][0] = toX;		grounds[groundsEnd].points.points[3][1] = toY + 0.5;
	grounds[groundsEnd].points.n = 4;
	grounds[groundsEnd].points.velocity[0] = 0.0;		grounds[groundsEnd].points.velocity[1] = 0.0;
	grounds[groundsEnd].points.usev = 1;
	DrawerAdd(GroundDrawer, groundsEnd, 6);
	CollisionAdd(GroundCollision, groundsEnd, ID_GROUND, &groundCollisionTypes, GroundCollisionNotifier);
	++groundsEnd;
	return groundsEnd;
}
static int removedID;
static int GroundTimerUpdateID(int id)
{
	if (removedID == -1)
		return -1;
	if (id > removedID)
		return id - 1;
	if (id == removedID)
		return -1;
	return id;
}

void GroundRemove(int id)
{
	int i;
	groundsMostId[0] = groundsMostId[1] = 0;
	groundsLowestID = 0;
	for (i = 0; i < groundsEnd; ++i) {
		if (i == id) continue;
		if (grounds[i].leftId == id)
			grounds[i].leftId = -1;
		else if (grounds[i].leftId > id)
			grounds[i].leftId -= 1;
		if (grounds[i].rightId == id)
			grounds[i].rightId = -1;
		else if (grounds[i].rightId > id)
			grounds[i].rightId -= 1;
		if (grounds[i].fromX < grounds[groundsMostId[0]].fromX)
			groundsMostId[0] = i;
		if (grounds[i].toX > grounds[groundsMostId[1]].toX)
			groundsMostId[1] = i;
		if (max(grounds[groundsLowestID].fromY, grounds[groundsLowestID].toY) < max(grounds[i].fromY, grounds[i].toY))
			groundsLowestID = i;
	}
	--groundsEnd;
	for (i = id; i <groundsEnd; ++i)
		memcpy(&grounds[i], &grounds[i + 1], sizeof(grounds[0]));
	if (groundsEnd != 0) {
		if (groundsMostId[0] > id) groundsMostId[0] -= 1;
		if (groundsMostId[1] > id) groundsMostId[1] -= 1;
		if (groundsLowestID > id) groundsLowestID -= 1;
	}
	else
		groundsMostId[0] = groundsMostId[1] = groundsLowestID = -1;
	removedID = id;
	TimerUpdateID(GroundVisibleTimer, GroundTimerUpdateID);
	DrawerRemove(GroundDrawer, groundsEnd);
	CollisionRemove(GroundCollision, groundsEnd);
}
void GroundClear()
{
	int i;
	for (i = 0; i < groundsEnd; ++i) {
		DrawerRemove(GroundDrawer, i);
		CollisionRemove(GroundCollision, i);
	}
	removedID = -1;
	TimerUpdateID(GroundVisibleTimer, GroundTimerUpdateID);
	groundsEnd = 0;
	groundsMostId[0] = groundsMostId[1] = groundsLowestID = -1;
}
static int GroundRemoveCommand(wchar_t *command)
{
	int id;
	if (swscanf(command, L"%*s%d", &id) && id > 0 && id < groundsEnd) {
		GroundRemove(id);
		return 0;
	}
	return 1;
}
static int GroundClearCommand(wchar_t *command)
{
	GroundClear();
	return 0;
}
static int GroundCreate(wchar_t *command)
{
	double fromX, fromY, toX, toY;
	int traits, invisible;
	if (groundsEnd == MAX_GROUNDLEN) return 1;
	if (swscanf(command, L"%*s%lf%lf%lf%lf%d%d", &fromX, &fromY, &toX, &toY, &traits, &invisible) == 6 && toX > fromX)
		return GroundAdd(fromX, fromY, toX, toY, traits, invisible) == -1 ? 1 : 0;
	return 1;
}
void GroundInit()
{
	backgroundColor = RGB(0xff, 0xff, 0xff);
	hBrushBackground = CreateSolidBrush(backgroundColor);
	hPenBorder = CreatePen(PS_SOLID, 1, COLOR_BORDER);
	hPenNoOutline = CreatePen(PS_NULL, 0, 0);
	DrawerAdd(BackgroundDrawer, 0, 11);
	groundsMostId[0] = groundsMostId[1] = groundsLowestID = -1;
	LoaderAdd(L"border", BorderCreate);
	LoaderAdd(L"gravity", GravityCreate);
	LoaderAdd(L"friction", FrictionCreate);
	LoaderAdd(L"background", BackgroundCreate);
	LoaderAdd(L"ground", GroundCreate);
	LoaderAdd(L"groundremove", GroundRemoveCommand);
	LoaderAdd(L"groundclear", GroundClearCommand);
}
void GroundDestroy()
{
	DeleteObject(hPenNoOutline);
	DeleteObject(hPenBorder);
	DeleteObject(hBrushBackground);
}
void GroundStart()
{
	int i;
	for (i = 0; i < bordersEnd; ++i) {
		if (borders[i].visible)
			DrawerAdd(BorderDrawer, i, 0);
		CollisionAdd(BorderCollision, i, ID_BORDER, &borderCollisionTypes, BorderCollisionNotifier);
	}
}
void GroundStop()
{
	int i;
	DeleteObject(hBrushBackground);
	for (i = 0; i < bordersEnd; ++i) {
		if (borders[i].visible)
			DrawerRemove(BorderDrawer, i);
		CollisionRemove(BorderCollision, i);
	}
	bordersEnd = 0;
	GroundClear();
	groundFriction = 0.0;
	groundGravity = 0.0;
	DeleteObject(hBrushBackground);
	backgroundColor = RGB(0xff, 0xff, 0xff);
	hBrushBackground = CreateSolidBrush(backgroundColor);
}
void GroundPause() {}
void GroundResume() {}
