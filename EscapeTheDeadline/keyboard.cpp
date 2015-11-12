#include <string.h>
#include "keyboard.h"


int KeyboardIsDown[INVALID];
int KeyboardGetNum[INVALID];

void KeyboardClear()
{
	memset(KeyboardGetNum, 0, sizeof(KeyboardGetNum));
}

void KeyboardInit() {}
void KeyboardDestoy() {}

static enum Button KeyDecode(WPARAM wParam)
{
	//LEFT = 0, UP, RIGHT, DOWN, JUMP, PAUSE, INVALID
	switch (wParam)
	{
	case VK_LEFT:  case 'A':
		return LEFT;
	case VK_UP:    case 'W':
		return UP;
	case VK_RIGHT: case 'D':
		return RIGHT;
	case VK_DOWN:  case 'S':
		return DOWN;
	case VK_SPACE:
		return JUMP;
	case 'P':
		return PAUSE;
	}
	return INVALID;
}

void KeyboardKeyDown(WPARAM wParam)
{
	enum Button keycode = KeyDecode(wParam);
	if (keycode == INVALID) return;
	KeyboardIsDown[keycode] = 1;
	KeyboardGetNum[keycode] += 1;
}

void KeyboardKeyUp(WPARAM wParam)
{
	enum Button keycode = KeyDecode(wParam);
	if (keycode == INVALID) return;
	KeyboardIsDown[keycode] = 0;
}
