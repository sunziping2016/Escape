#include <string.h>
#include "keyboard.h"

int KeyboardIsDown[MAX_VK];
int KeyboardGetNum[MAX_VK];

void KeyboardClear()
{
	memset(KeyboardGetNum, 0, sizeof(KeyboardGetNum));
}

void KeyboardInit() {}
void KeyboardDestoy() {}

void KeyboardKeyDown(WPARAM wParam)
{
	if (wParam >= MAX_VK) return;
	KeyboardIsDown[wParam] = 1;
	KeyboardGetNum[wParam] += 1;
}

void KeyboardKeyUp(WPARAM wParam)
{
	if (wParam >= MAX_VK) return;
	KeyboardIsDown[wParam] = 0;
}
