#include <string.h>
#include "keyboard.h"

int KeyboardIsDown[MAX_VK];
int KeyboardGetNum[MAX_VK];

void KeyboardClear()
{
	//memset(KeyboardIsDown, 0, sizeof(KeyboardIsDown));
	memset(KeyboardGetNum, 0, sizeof(KeyboardGetNum));
}
#define MAX_PROCESSERSLEN		30
static struct {
	void(*func)(int id, WPARAM wParam, int keytype);
	int id;
} processers[MAX_PROCESSERSLEN];
static int processersEnd;

void KeyboardInit() {}
void KeyboardDestoy() {}
int KeyboardAdd(void(*func)(int id, WPARAM wParam, int keytype), int id)
{
	if (processersEnd == MAX_PROCESSERSLEN)	return 1;
	processers[processersEnd].func = func;
	processers[processersEnd].id = id;
	++processersEnd;
	return 0;
}
void KeyboardRemove(void(*func)(int id, WPARAM wParam, int keytype), int id)
{
	int pos, i;
	for (pos = 0; pos < processersEnd; ++pos)
		if (processers[pos].func == func && processers[pos].id == id)
			break;
	if (pos == processersEnd) return;
	--processersEnd;
	for (i = pos; i < processersEnd; i++) {
		processers[i].func = processers[i + 1].func;
		processers[i].id = processers[i + 1].id;
	}

}
void KeyboardKeyDown(WPARAM wParam)
{
	int i;
	if (wParam >= MAX_VK) return;
	KeyboardIsDown[wParam] = 1;
	KeyboardGetNum[wParam] += 1;
	for (i = 0; i < processersEnd; ++i)
		(*processers[i].func)(processers[i].id, wParam, KEYDOWN);
}
void KeyboardKeyUp(WPARAM wParam)
{
	int i;
	if (wParam >= MAX_VK) return;
	KeyboardIsDown[wParam] = 0;
	for (i = 0; i < processersEnd; ++i)
		(*processers[i].func)(processers[i].id, wParam, KEYUP);
}
void KeyboardKeyChar(WPARAM wParam)
{
	int i;
	if (wParam >= MAX_VK) return;
	for (i = 0; i < processersEnd; ++i)
		(*processers[i].func)(processers[i].id, wParam, KEYCHAR);
}
