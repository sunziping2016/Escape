#pragma once
#include <windows.h>
#include "configure.h"

// To create a timer with changing frequency to save CPU

void TimerInit();
void TimerDestroy();

int TimerAdd(void(*func)(int, int), int id, int ms);

int TimerProcess(HWND hWnd);
