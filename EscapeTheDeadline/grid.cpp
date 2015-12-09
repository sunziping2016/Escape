#pragma warning(disable: 4996)
#include <windows.h>
#include <math.h>
#include <string.h>
#include "engine.h"
#include "grid.h"
#include "drawer.h"
#include "loader.h"
#include "world.h"

#define COLOR_GRID		RGB(0xe0, 0xe0, 0xe0)
#define COLOR_TEXT		RGB(0xa9, 0xa9, 0xa9)
#define WIDTH_GRID		2
#define FONTPADDING		8
#define FONTSIZE_GRID	20
#define FONTNAME_GRID	TEXT("Monotype Corsiva")

static int hasGridV, hasGridH;
static double gapV, gapH;

static HPEN hPen;
static HFONT hFont;

#define BUFFER_SIZE		20

static void GridDrawerV(int id, HDC hDC)
{
	double y, end;
	int len;
	SIZE size;
	wchar_t buffer[BUFFER_SIZE];
	if (gameState != STARTED && hasGridV) return;
	WorldSetViewport(0.0, 0.0);
	end = viewY + DrawerY / 2.0;
	SelectObject(hDC, hPen);
	SelectObject(hDC, hFont);
	SetTextColor(hDC, COLOR_TEXT);
	for (y = floor((viewY - DrawerY / 2.0) / gapV) * gapV; y < end; y += gapV) {
		swprintf(buffer, L"%.0f", -y);
		len = (int)wcslen(buffer);
		GetTextExtentPoint(hDC, buffer, len, &size);
		MoveToEx(hDC, 0, WorldY(y), NULL);
		LineTo(hDC, DrawerX - size.cx - 2 * FONTPADDING, WorldY(y));
		TextOut(hDC, DrawerX - size.cx - FONTPADDING, WorldY(y) - size.cy / 2, buffer, len);
	}
}
static void GridDrawerH(int id, HDC hDC)
{
	double x, end;
	int len;
	SIZE size;
	wchar_t buffer[BUFFER_SIZE];
	if (gameState != STARTED && hasGridH) return;
	WorldSetViewport(0.0, 0.0);
	end = viewX + DrawerX / 2.0;
	SelectObject(hDC, hPen);
	SelectObject(hDC, hFont);
	SetTextColor(hDC, COLOR_TEXT);
	for (x = floor((viewX - DrawerX / 2.0) / gapH) * gapH; x < end; x += gapH) {
		swprintf(buffer, L"%.0f", x);
		len = (int)wcslen(buffer);
		GetTextExtentPoint(hDC, buffer, len, &size);
		MoveToEx(hDC, WorldX(x), 2 * FONTPADDING + size.cy, NULL);
		LineTo(hDC, WorldX(x), DrawerY);
		TextOut(hDC, WorldX(x) - size.cx / 2, FONTPADDING, buffer, len);
	}
}
static int GridCreaterV(wchar_t *command)
{
	if (swscanf(command, L"%*s%lf", &gapV) && gapV > 1.0) {
		hasGridV = 1;
		return 0;
	}
	return 1;
}
static int GridCreaterH(wchar_t *command)
{
	if (swscanf(command, L"%*s%lf", &gapH) && gapH > 1.0) {
		hasGridH = 1;
		return 0;
	}
	return 1;
}
void GridInit()
{
	hPen = CreatePen(PS_SOLID, WIDTH_GRID, COLOR_GRID);
	hFont = CreateFont(FONTSIZE_GRID, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_GRID);
	DrawerAdd(GridDrawerV, 0, 10);
	DrawerAdd(GridDrawerH, 0, 10);
	LoaderAdd(L"gridv", GridCreaterV);
	LoaderAdd(L"gridh", GridCreaterH);
}
void GridDestroy()
{
	DeleteObject(hFont);
	DeleteObject(hPen);
}
void GridStart() {}
void GridStop()
{
	hasGridV = hasGridH = 0;
	gapV = gapH = 0.0;
}
