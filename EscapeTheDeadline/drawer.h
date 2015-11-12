#pragma once
#include <windows.h>
#include "configure.h"

extern int DrawerX, DrawerY;

void DrawerInit(HWND hWnd);
void DrawerDestroy();
void DrawerUpdateSize(HWND hWnd);
void DrawerProcess(HDC hdc);

int DrawerAdd(void(*func)(HDC, int), int id, int priority);
void DrawerRemove(void(*func)(HDC, int), int id);
