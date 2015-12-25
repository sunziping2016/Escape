#pragma once
#include <windows.h>

// To create a timer with changing frequency to save CPU

void TimerInit();
void TimerDestroy();

int TimerAdd(void(*func)(int id, int ms), int id, int ms);
void TimerUpdateID(void(*func)(int id, int ms), int (*updateId)(int id));
int TimerProcess(HWND hWnd);
