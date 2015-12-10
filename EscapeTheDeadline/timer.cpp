#include "timer.h"
#include "assert.h"

#define MAX_TIMERLEN	200
#define MAX_TIMERELAPSE	20

#define ID_TIMER 1

#define prevIter(iter) ((iter + MAX_TIMERLEN - 1) % MAX_TIMERLEN)
#define nextIter(iter) ((iter + 1) % MAX_TIMERLEN)
#define midIter(iter1, iter2) (iter1 <= iter2 ? (iter1 + iter2) / 2 : (iter1 + iter2 + MAX_TIMERLEN) / 2 % MAX_TIMERLEN)

static struct {
	void(*func)(int, int);
	int id;
	int ms;
} timers[MAX_TIMERLEN];
static int timersBegin, timersEnd;

void TimerInit()
{
	timersBegin = timersEnd = 0;
}
void TimerDestroy() {}

static void copyTimer(int to, int from)
{
	timers[to].func = timers[from].func;
	timers[to].id = timers[from].id;
	timers[to].ms = timers[from].ms;
}

int TimerAdd(void(*func)(int id, int ms), int id, int ms)
{
	int low = timersBegin, high = prevIter(timersEnd), i;
	if (timersBegin == nextIter(timersEnd)) return 1;
	while (low != high)
	{
		int i = midIter(low, high);
		if (ms > timers[i].ms)
			low = nextIter(i);
		else
			high = i;
	}
	if (nextIter(low) == timersEnd && ms > timers[low].ms)
		low = timersEnd;
	for (i = timersEnd; i != low; i = prevIter(i))
		copyTimer(i, prevIter(i));
	timers[low].func = func;
	timers[low].id = id;
	timers[low].ms = ms;
	timersEnd = nextIter(timersEnd);
	return 0;
}

static int msHash(int ms)
{
	return (ms + MAX_TIMERELAPSE / 2) / MAX_TIMERELAPSE * MAX_TIMERELAPSE;
}

int TimerProcess(HWND hWnd)
{
	int i, ms;
	//long time = GetTickCount();
	//LogPrintf("%ld:	TimerProcess Start:\n", time);
	if (timersBegin == timersEnd) return 1;
	while(timersBegin != timersEnd) {
		//ms = msHash(timers[timersBegin].ms + (GetTickCount() - time));
		ms = msHash(timers[timersBegin].ms);
		if (ms > 0) break;
		(*timers[timersBegin].func)(timers[timersBegin].id, timers[timersBegin].ms);
		timersBegin = nextIter(timersBegin);
	}
	SetTimer(hWnd, ID_TIMER, ms, NULL);
	for (i = timersBegin; i != timersEnd; i = nextIter(i))
		timers[i].ms -= ms;
	//LogPrintf("%ld:	TimerProcess End: %d\n", time, ms);
	return 0;
}
