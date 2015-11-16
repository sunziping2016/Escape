#include <math.h>

#include "keyboard.h"
#include "drawer.h"
#include "timer.h"
#include "collision.h"
#include "test.h"

#define WALL	0
#define SQUARE	1

struct {
	double x, y;
	double vx, vy;
	int ms;
} mS[1];

HBRUSH hb = CreateSolidBrush(0);

Points *CollisionBorder(int id)
{
	static Points points;
	double border[][2] = { {0, 0}, { 0, DrawerY / 2 }, { DrawerX / 2, DrawerY },  { DrawerX, DrawerY }, { DrawerX, 0 }, {0, 0} };
	points.points[0][0] = border[id][0];
	points.points[0][1] = border[id][1];
	points.points[1][0] = border[id + 1][0];
	points.points[1][1] = border[id + 1][1];
	points.n = 2;
	return &points;
}


Points *CollisionMS(int id)
{
	static Points points; 
	points.points[0][0] = mS[id].x;		 points.points[0][1] = mS[id].y;
	points.points[1][0] = mS[id].x + 10; points.points[1][1] = mS[id].y;
	points.points[2][0] = mS[id].x + 10; points.points[2][1] = mS[id].y + 10;
	points.points[3][0] = mS[id].x;		 points.points[3][1] = mS[id].y + 10;
	points.points[4][0] = mS[id].x;		 points.points[4][1] = mS[id].y;
	points.n = 5;
	return &points;
}

void CollisionMSNotifier(int id, int othertype, int otherid, double n[2], double depth)
{
	double newvx, newvy;
	newvx = -(2 * mS[id].vy * n[0] * n[1] - mS[id].vx * (n[1] * n[1] - n[0] * n[0]));
	newvy = -(2 * mS[id].vx * n[0] * n[1] + mS[id].vy * (n[1] * n[1] - n[0] * n[0]));
	mS[id].vx = newvx;
	mS[id].vy = newvy;
	mS[id].x += -depth * n[0];
	mS[id].y += -depth * n[1];
}

void DrawLine(int id, HDC hDC)
{
	POINT points[3] = { { 0 , DrawerY },{ 0 , DrawerY / 2 },{ DrawerX / 2 , DrawerY } };
	HBRUSH hBrush = CreateSolidBrush(RGB(0x88, 0x88, 0x88));
	SelectObject(hDC, hBrush);
	Polygon(hDC, points, 3);
	DeleteObject(hBrush);
}

void DrawMS(int id, HDC hDC)
{
	RECT rect = { lround(mS[id].x), lround(mS[id].y), lround(mS[id].x + 10), lround(mS[id].y + 10) };
	FillRect(hDC, &rect, hb);
}

void TimerMS(int id, int ms)
{
	mS[id].vx += 2 * (KeyboardGetNum[VK_RIGHT] - KeyboardGetNum[VK_LEFT]);
	mS[id].vy += 2 * (KeyboardGetNum[VK_DOWN] - KeyboardGetNum[VK_UP]) + 1;
	if (KeyboardIsDown['P'])
		mS[id].vx = mS[id].vy = 0;
	mS[id].x += mS[id].vx;
	mS[id].y += mS[id].vy;
	KeyboardClear();
	TimerAdd(TimerMS, id, mS[id].ms + ms);
}

void TestInit()
{
	int i = 0;
	for (i = 0; i < 5; ++i)
		CollisionAdd(CollisionBorder, i, WALL, NULL, NULL);
	static Types types = { { WALL }, 1 };
	for (i = 0; i < 1; ++i) {
		mS[i].x = 50;
		mS[i].y = 50;
		mS[i].vx = 0;
		mS[i].vy = 0;
		mS[i].ms = 20;
		DrawerAdd(DrawMS, i, 0);
		TimerAdd(TimerMS, i, mS[i].ms);
		CollisionAdd(CollisionMS, i, SQUARE, &types, CollisionMSNotifier);
	}
	DrawerAdd(DrawLine, 0, 0);
}

void TestDestroy() {}
