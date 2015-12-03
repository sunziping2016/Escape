#pragma once
#include <windows.h>

extern int DrawerX, DrawerY;

void DrawerInit(HWND hWnd);
void DrawerDestroy();

int DrawerAdd(void(*func)(int id, HDC hDC), int id, int priority);
void DrawerRemove(void(*func)(int id, HDC hDC), int id);
void DrawerUpdateSize(HWND hWnd);
void DrawerProcess(HDC hdc);

#define DrawerColor(a, b, r)		RGB((int)(r * GetRValue(a) + (1 - r) * GetRValue(b)), \
									(int)(r * GetGValue(a) + (1 - r) * GetGValue(b)), \
									(int)(r * GetBValue(a) + (1 - r) * GetBValue(b)))
#define DrawerRGB(c)				((c & 0xff) << 16 | (c & 0xff << 8) | (c & 0xff << 16) >> 16)

void DrawerAlphaColor(HDC hDest, int xDest, int yDest, int w, int h, COLORREF color, double r);
void DrawerAlphaBitmap(HDC hDest, int xDest, int yDest, int w, int h, HDC hSrc, int xSrc, int ySrc, COLORREF transparent, double r);
