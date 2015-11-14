#include "drawer.h"

int DrawerX, DrawerY;
static HDC hDCbuf;
static HBITMAP hBitmap;
static HBRUSH hBrushBackground;

static struct {
	void(*func)(HDC, int);
	int id;
	int priority;
} drawers[MAX_DRAWLEN];
static int drawersEnd;

static void UpdateSize(HWND hWnd)
{
	RECT rect;
	GetClientRect(hWnd, &rect);
	if (rect.right == 0 || rect.bottom == 0) return; // To make hBitmap valid
	DrawerX = rect.right;
	DrawerY = rect.bottom;
}

void DrawerInit(HWND hWnd)
{
	HDC hDC = GetDC(hWnd);
	UpdateSize(hWnd);
	hDCbuf = CreateCompatibleDC(hDC);
	hBitmap = CreateCompatibleBitmap(hDC, DrawerX, DrawerY);
	SelectObject(hDCbuf, hBitmap);
	hBrushBackground = CreateSolidBrush(RGB(0xff, 0xff, 0xff));
	ReleaseDC(hWnd, hDC);
	drawersEnd = 0;
}

void DrawerDestroy()
{
	DeleteObject(hBrushBackground);
	DeleteObject(hBitmap);
	DeleteDC(hDCbuf);
}

void DrawerUpdateSize(HWND hWnd)
{
	UpdateSize(hWnd);
	DeleteObject(hBitmap);
	hBitmap = CreateCompatibleBitmap(hDCbuf, DrawerX, DrawerY);
	SelectObject(hDCbuf, hBitmap);
}
void DrawerProcess(HDC hDC)
{
	int i;
	RECT rect = { 0, 0, DrawerX, DrawerY };
	FillRect(hDCbuf, &rect, hBrushBackground);
	for (i = 0; i < drawersEnd; ++i)
		(*drawers[i].func)(hDCbuf, drawers[i].id);
	BitBlt(hDC, 0, 0, DrawerX, DrawerY, hDCbuf, 0, 0, SRCCOPY);
}

static void copyDrawer(int to, int from)
{
	drawers[to].func = drawers[from].func;
	drawers[to].id = drawers[from].id;
	drawers[to].priority = drawers[from].priority;
}

int DrawerAdd(void(*func)(HDC, int), int id, int priority)
{
	int low = 0, high = drawersEnd - 1, i;
	if (drawersEnd == MAX_DRAWLEN) return 1;
	while (low < high)
	{
		int i = (low + high) / 2;
		if (priority < drawers[i].priority)
			low = i + 1;
		else
			high = i;
	}
	if (low + 1 == drawersEnd && priority < drawers[low].priority)
		low = drawersEnd;
	for (i = drawersEnd; i > low; --i)
		copyDrawer(i, i - 1);
	drawers[low].func = func;
	drawers[low].id = id;
	drawers[low].priority = priority;
	++drawersEnd;
	return 0;
}
void DrawerRemove(void(*func)(HDC, int), int id) // OPTIMIZE
{
	int pos, i;
	for (pos = 0; pos < drawersEnd; ++pos)
		if (drawers[pos].func == func && drawers[pos].id == id)
			break;
	if (pos == drawersEnd) return;
	--drawersEnd;
	for (i = pos; i < drawersEnd; i++)
		copyDrawer(i, i + 1);
}
