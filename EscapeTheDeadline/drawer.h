#pragma once
#include <windows.h>

extern int DrawerX, DrawerY;

void DrawerInit(HWND hWnd);
void DrawerDestroy();

int DrawerAdd(void(*func)(int id, HDC hDC), int id, int priority);
void DrawerRemove(void(*func)(int id, HDC hDC), int id);
void DrawerUpdateSize(HWND hWnd);
void DrawerProcess(HDC hdc);
