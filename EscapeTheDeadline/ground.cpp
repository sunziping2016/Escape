#include <stdio.h>
#include "ground.h"
#include "drawer.h"
#include "loader.h"
#include "engine.h"
#include "collision.h"
#include "world.h"

#define MAX_BORDERLEN	10

#define WIDTH_BORDER	6
#define COLOR_BORDER	RGB(0x80, 0x80, 0x80)

double groundFriction;
double groundGravity;
COLORREF groundColor;

static HBRUSH hBrushBackground;

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
	if (swscanf(command, L"%*s%lx", &groundColor) == 1)
		groundColor = DrawerRGB(groundColor);
		return 0;
	return 1;
}
static void BorderDrawer(int id, HDC hDC)
{
	POINT points[4];
	double len, newx, newy, deltax, deltay;
	HBRUSH hBrush;
	HPEN hPen;
	if (gameState != STARTED) return;
	WorldSetViewport(borders[id].fromX, borders[id].fromY);
	deltax = borders[id].toX - borders[id].fromX; deltay = borders[id].toY - borders[id].fromY;
	len = sqrt(deltax * deltax + deltay * deltay);
	newx = -WIDTH_BORDER / len * deltay; newy = WIDTH_BORDER / len * deltax;
	points[0].x = WorldX(-WIDTH_BORDER / len * deltax);					points[0].y = WorldY(-WIDTH_BORDER / len * deltay);
	points[1].x = WorldX(WIDTH_BORDER / len * deltax + deltax);			points[1].y = WorldY(WIDTH_BORDER / len * deltay + deltay);
	points[2].x = WorldX(WIDTH_BORDER / len * deltax + deltax + newx);	points[2].y = WorldY(WIDTH_BORDER / len * deltay + deltay + newy);
	points[3].x = WorldX(-WIDTH_BORDER / len * deltax + newx);			points[3].y = WorldY(-WIDTH_BORDER / len * deltay + newy);
	hPen = CreatePen(PS_SOLID, 0, COLOR_BORDER);
	hBrush = CreateSolidBrush(COLOR_BORDER);
	SelectObject(hDC, hPen);
	SelectObject(hDC, hBrush);
	Polygon(hDC, points, 4);
	DeleteObject(hPen);
	hPen = CreatePen(PS_SOLID, WIDTH_BORDER / 3, RGB(0, 0, 0));
	SelectObject(hDC, hPen);
	MoveToEx(hDC, points[0].x, points[0].y, NULL);
	LineTo(hDC, points[1].x, points[1].y);
	DeleteObject(hBrush);
	DeleteObject(hPen);
}
static Points *BorderCollision(int id)
{
	static Points points;
	points.points[0][0] = borders[id].fromX;	points.points[0][1] = borders[id].fromY;
	points.points[1][0] = borders[id].toX;		points.points[1][1] = borders[id].toY;
	points.n = 2;
	return &points;
}
static void BackgroundDrawer(int id, HDC hDC)
{
	RECT rect;
	if (gameState != STARTED) return;
	rect.left = 0;			rect.top = 0;
	rect.right = DrawerX;	rect.bottom = DrawerY;
	FillRect(hDC, &rect, hBrushBackground);
}
void GroundInit()
{
	groundColor = RGB(0xff, 0xff, 0xff);
	DrawerAdd(BackgroundDrawer, 0, 11);
	LoaderAdd(L"border", BorderCreate);
	LoaderAdd(L"gravity", GravityCreate);
	LoaderAdd(L"friction", FrictionCreate);
	LoaderAdd(L"background", BackgroundCreate);
}
void GroundDestroy() {}
void GroundStart()
{
	int i;
	for (i = 0; i < bordersEnd; ++i) {
		if (borders[i].visible)
			DrawerAdd(BorderDrawer, i, 0);
		CollisionAdd(BorderCollision, i, ID_BORDER, NULL, NULL);
	}
	hBrushBackground = CreateSolidBrush(groundColor);
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
	groundFriction = 0.0;
	groundGravity = 0.0;
	groundColor = RGB(0xff, 0xff, 0xff);
}
void GroundPause() {}
void GroundResume() {}
