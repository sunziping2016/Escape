#include <math.h>

#include "keyboard.h"
#include "drawer.h"
#include "timer.h"
#include "collision.h"
#include "world.h"
#include "test.h"
#include "engine.h"

#define WALL	0
#define SQUARE	1

struct {
	double x, y;
	double vx, vy;
	int ms;
	Points points;
} mS[150];

HBRUSH hb = CreateSolidBrush(0);
HBRUSH hred = CreateSolidBrush(RGB(255, 0, 0));

Points *CollisionBorder(int id)
{
	static Points points;
	static double border[][2] = { {0.0, 0.0}, { 0.0, (double)DrawerY / 2.0 }, { (double)DrawerX, (double)DrawerY }, { (double)DrawerX, 0.0 }, {0.0, 0.0} };
	points.points[0][0] = border[id][0];
	points.points[0][1] = border[id][1];
	points.points[1][0] = border[id + 1][0];
	points.points[1][1] = border[id + 1][1];
	points.n = 2;
	return &points;
}

Points *CollisionMS(int id)
{
	mS[id].points.points[0][0] = mS[id].x;		 mS[id].points.points[0][1] = mS[id].y;
	mS[id].points.points[1][0] = mS[id].x + 10;	 mS[id].points.points[1][1] = mS[id].y;
	mS[id].points.points[2][0] = mS[id].x + 10;  mS[id].points.points[2][1] = mS[id].y + 10;
	mS[id].points.points[3][0] = mS[id].x;		 mS[id].points.points[3][1] = mS[id].y + 10;
	mS[id].points.points[4][0] = mS[id].x;		 mS[id].points.points[4][1] = mS[id].y;
	mS[id].points.n = 5;
	return &mS[id].points;
}

void CollisionMSNotifier(int id, int othertype, int otherid, double n[2], double depth)
{
	double newvx, newvy;
	//if (id == 0) return;
	newvx = -(2 * mS[id].vy * n[0] * n[1] - mS[id].vx * (n[1] * n[1] - n[0] * n[0]));
	newvy = -(2 * mS[id].vx * n[0] * n[1] + mS[id].vy * (n[1] * n[1] - n[0] * n[0]));
	mS[id].vx = 0.2 * newvx;
	mS[id].vy = 0.2 * newvy;
	mS[id].x += -depth * n[0];
	mS[id].y += -depth * n[1];
}

void DrawLine(int id, HDC hDC)
{
	if (gameState != STARTED) return;
	POINT point = WorldSetMapper(hDC, 0, 0);
	static POINT points[3] = { { 0 , DrawerY },{ 0 , DrawerY / 2 },{ DrawerX , DrawerY } };
	HBRUSH hBrush = CreateSolidBrush(RGB(0x88, 0x88, 0x88));
	SelectObject(hDC, hBrush);
	Polygon(hDC, points, 3);
	DeleteObject(hBrush);
	WorldResetMapper(hDC, &point);
}

void DrawMS(int id, HDC hDC)
{
	if (gameState != STARTED) return;
	POINT point = WorldSetMapper(hDC, mS[id].x, mS[id].y);
	RECT rect = { 0, 0, 10, 10};
	FillRect(hDC, &rect, id == 0 ? hred : hb);
	WorldResetMapper(hDC, &point);
}

void TimerMS(int id, int ms)
{
	if (gameState != STARTED || gamePaused) return;
	if (KeyboardIsDown[VK_ESCAPE]) EnginePause();
	if (id == 0) {
		mS[id].vx += 1 * (KeyboardGetNum[VK_RIGHT] - KeyboardGetNum[VK_LEFT]);
		//mS[id].vy += 10 * (KeyboardGetNum[VK_DOWN] - KeyboardGetNum[VK_UP]) + 1;
		mS[id].vy -= 20 * KeyboardGetNum[VK_SPACE] - 1;
		//KeyboardClear();
	}
	else
		mS[id].vy += 1;
	if (KeyboardIsDown['P'])
		mS[id].vx = mS[id].vy = 0;
	mS[id].x += mS[id].vx;
	mS[id].y += mS[id].vy;
	TimerAdd(TimerMS, id, mS[id].ms + ms);
}

void Tracker(int id, double *x, double *y)
{
	*x = mS[id].x;
	*y = mS[id].y;
}

void TestInit()
{
	int i = 0;
	for (i = 0; i < 4; ++i)
		CollisionAdd(CollisionBorder, i, WALL, NULL, NULL);
	static Types types = { { WALL, SQUARE}, 2 };
	for (i = 0; i < 1; ++i) {
		DrawerAdd(DrawMS, i, 10);
		CollisionAdd(CollisionMS, i, SQUARE, &types, CollisionMSNotifier);
	}
	DrawerAdd(DrawLine, 0, 0);
}
void TestDestroy() {}
void TestStart()
{
	int i;
	for (i = 0; i < 1; ++i) {
		mS[i].x = 20 + 20 * (i % 30);
		mS[i].y = 50 + 40 * (i / 30);
		mS[i].vx = 0;
		mS[i].vy = 0;
		mS[i].ms = 20;
	}
	WorldSetTracked(Tracker, 0);
	TestResume();
}
void TestStop() {}
void TestResume()
{
	int i;
	for (i = 0; i < 1; ++i)
		TimerAdd(TimerMS, i, mS[i].ms);
}
void TestPause() {}
