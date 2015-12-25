#pragma once
#include <windows.h>

#define MAX_VK 0x100

extern int KeyboardIsDown[MAX_VK];
extern int KeyboardGetNum[MAX_VK];

enum KeyType { KEYDOWN, KEYUP, KEYCHAR };

void KeyboardInit();
void KeyboardDestoy();
int KeyboardAdd(void(*func)(int id, WPARAM wParam, int keytype), int id);
void KeyboardRemove(void(*func)(int id, WPARAM wParam, int keytype), int id);

void KeyboardClear(); // DEPREACTED
void KeyboardKeyDown(WPARAM wParam);
void KeyboardKeyUp(WPARAM wParam);
void KeyboardKeyChar(WPARAM wParam);
