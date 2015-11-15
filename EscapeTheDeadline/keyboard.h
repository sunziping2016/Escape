#pragma once
#include <windows.h>

#define MAX_VK 0x100

extern int KeyboardIsDown[MAX_VK];
extern int KeyboardGetNum[MAX_VK];

void KeyboardInit();
void KeyboardDestoy();

void KeyboardClear(); // DEPREACTED
void KeyboardKeyDown(WPARAM wParam);
void KeyboardKeyUp(WPARAM wParam);
