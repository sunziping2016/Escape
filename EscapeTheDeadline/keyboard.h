#pragma once
#include <windows.h>

enum Button { LEFT = 0, UP, RIGHT, DOWN, JUMP, PAUSE, INVALID };

extern int KeyboardIsDown[INVALID];
extern int KeyboardGetNum[INVALID];

void KeyboardInit();
void KeyboardDestoy();

void KeyboardClear();

void KeyboardKeyDown(WPARAM wParam);
void KeyboardKeyUp(WPARAM wParam);
