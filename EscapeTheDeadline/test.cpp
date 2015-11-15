#include "keyboard.h"
#include "drawer.h"
#include "timer.h"
#include "test.h"


struct {
	int x, y;
	int vx, vy;
	int ms;
} mS[1];

HBRUSH hb = CreateSolidBrush(0);

void DrawMS(int id, HDC hDC)
{
	RECT rect = { mS[id].x, mS[id].y, mS[id].x + 10, mS[id].y + 10 };
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
	if (mS[id].x < 0) {
		mS[id].x = -mS[id].x;
		mS[id].vx = -mS[id].vx;
	}
	if (mS[id].x > DrawerX) {
		mS[id].x = 2 * DrawerX - mS[id].x;
		mS[id].vx = -mS[id].vx;
	}
	if (mS[id].y < 0) {
		mS[id].y = -mS[id].y;
		mS[id].vy = -mS[id].vy;
	}
	if (mS[id].y > DrawerY) {
		mS[id].y = 2 * DrawerY - mS[id].y;
		mS[id].vy = -mS[id].vy;
	}
	KeyboardClear();
	TimerAdd(TimerMS, id, mS[id].ms + ms);
}

void TestInit()
{
	int i = 0;
	for (i = 0; i < 1; ++i) {
		mS[i].x = 50;
		mS[i].y = 300;
		mS[i].vx = 0;
		mS[i].vy = 0;
		mS[i].ms = 20;
		DrawerAdd(DrawMS, i, 0);
		TimerAdd(TimerMS, i, mS[i].ms);
	}
}

void TestDestroy() {}
