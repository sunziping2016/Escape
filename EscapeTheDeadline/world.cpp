#include <math.h>
#include "drawer.h"
#include "timer.h"
#include "world.h"
#include "engine.h"

static double viewX, viewY;
static double viewVX, viewVY;

static void UpdateView(int id, int ms);
void WorldInit() {}
void WorldDestroy() {}

static void(*trackedFunc)(int id, double *x, double *y);
static int trackedID;

void WorldSetTracked(void(*func)(int id, double *x, double *y), int id)
{
	trackedFunc = func;
	trackedID = id;
}

#define factor1 0.03
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

void WorldMapper(double x, double y, int *newx, int *newy)
{
	*newx = (int)(x - viewX + 0.5) + DrawerX / 2;
	*newy = (int)(y - viewY + 0.5) + DrawerY / 2;
}

POINT WorldSetMapper(HDC hDC, double x, double y)
{
	POINT point;
	int newx, newy;
	WorldMapper(x, y, &newx, &newy);
	SetViewportOrgEx(hDC, newx, newy, &point);
	return point;
}
void WorldResetMapper(HDC hDC, POINT *orign)
{
	SetViewportOrgEx(hDC, orign->x, orign->y, NULL);
}
void WorldStart()
{
	viewX = DrawerX / 2;
	viewY = DrawerY / 2;
	WorldResume();
}
void WorldStop() {}
void WorldResume()
{
	TimerAdd(UpdateView, 0, 20);
}
void WorldPause() {}