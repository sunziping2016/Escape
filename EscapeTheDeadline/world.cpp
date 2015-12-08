#include <stdio.h>
#include <math.h>
#include "drawer.h"
#include "timer.h"
#include "world.h"
#include "engine.h"
#include "loader.h"

double viewX, viewY;
static double viewVX, viewVY;
static double viewportX, viewportY;

static void UpdateView(int id, int ms);
static int WorldCreate(wchar_t *command)
{
	if (swscanf(command, L"%*s%lf%lf", &viewX, &viewY) == 2)
		return 0;
	return 1;
}
void WorldInit()
{
	LoaderAdd(L"world", WorldCreate);
}
void WorldDestroy() {}

static void(*trackedFunc)(int id, double *x, double *y);
static int trackedID;

void WorldSetTracked(void(*func)(int id, double *x, double *y), int id)
{
	trackedFunc = func;
	trackedID = id;
}

#define factor1 0.12
#define factor2 (2 * sqrt(factor1))

static void UpdateView(int id, int ms)
{
	double trackedX, trackedY, ax, ay;
	if (gameState != STARTED || gamePaused) return;
	if (trackedFunc == NULL) {
		trackedX = DrawerX / 2;
		trackedY = DrawerY / 2;
	}
	else
		trackedFunc(trackedID, &trackedX, &trackedY);
	ax = factor1 * (trackedX - viewX) - factor2 * viewVX;
	ay = factor1 * (trackedY - viewY) - factor2 * viewVY;
	viewVX += ax;
	viewVY += ay;
	viewX += viewVX;
	viewY += viewVY;
	TimerAdd(UpdateView, id, ms + 20);
}

int WorldX(double x)
{
	return ROUND(x + viewportX - viewX) + DrawerX / 2;
}
int WorldY(double y)
{
	return ROUND(y + viewportY - viewY) + DrawerY / 2;
}
void WorldSetViewport(double x, double y)
{
	viewportX = x;
	viewportY = y;
}
void WorldStart()
{
	viewVX = viewVY = 0.0;
	viewportX = viewportY = 0.0;
	WorldResume();
}
void WorldStop()
{
	trackedFunc = NULL;
	trackedID = 0;
	viewX = viewY = 0.0;
}
void WorldResume()
{
	TimerAdd(UpdateView, 0, 20);
}
void WorldPause() {}
