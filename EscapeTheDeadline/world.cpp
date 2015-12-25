#include <stdio.h>
#include <math.h>
#include "drawer.h"
#include "timer.h"
#include "world.h"
#include "engine.h"
#include "loader.h"
#include "collision.h"

double viewX, viewY;
double viewRect[4];
static CollisionState viewerCollisionState;
static double viewVX, viewVY;
static double viewportX, viewportY;

static int WorldCreate(wchar_t *command)
{
	if (swscanf(command, L"%*s%lf%lf", &viewX, &viewY) == 2)
		return 0;
	return 1;
}

static void(*trackedFunc)(int id, double *x, double *y);
static int trackedID;

void WorldSetTracked(void(*func)(int id, double *x, double *y), int id)
{
	trackedFunc = func;
	trackedID = id;
}

#define factor1 0.06
#define factor2 (2 * sqrt(factor1))

void WorldProcess()
{
	double trackedX, trackedY, ax, ay;
	if (gameState != STARTED || gamePaused)	return;
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
	viewRect[0] = viewX - DrawerX / 2.0;		viewRect[1] = viewY - DrawerY / 2.0;
	viewRect[2] = viewX + DrawerX / 2.0;		viewRect[3] = viewY + DrawerY / 2.0;
	viewerCollisionState.points[0][0] = viewRect[0];	viewerCollisionState.points[0][1] = viewRect[1];
	viewerCollisionState.points[1][0] = viewRect[2];	viewerCollisionState.points[1][1] = viewRect[1];
	viewerCollisionState.points[2][0] = viewRect[2];	viewerCollisionState.points[2][1] = viewRect[3];
	viewerCollisionState.points[3][0] = viewRect[0];	viewerCollisionState.points[3][1] = viewRect[3];
	viewerCollisionState.points[4][0] = viewRect[0];	viewerCollisionState.points[4][1] = viewRect[1];
	viewerCollisionState.n = 5;
	viewerCollisionState.usev = 0;
}
static CollisionState *ViewerCollision(int id, int othertype, int otherid)
{
	return &viewerCollisionState;
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
void WorldInit()
{
	LoaderAdd(L"world", WorldCreate);
	CollisionAdd(ViewerCollision, 0, ID_VIEWER, NULL, NULL);
}
void WorldDestroy() {}
void WorldStart()
{
	viewVX = viewVY = 0.0;
	viewportX = viewportY = 0.0;
	viewRect[0] = viewX - DrawerX / 2.0;		viewRect[1] = viewY - DrawerY / 2.0;
	viewRect[2] = viewX + DrawerX / 2.0;		viewRect[3] = viewY + DrawerY / 2.0;
	WorldResume();
}
void WorldStop()
{
	trackedFunc = NULL;
	trackedID = 0;
	viewX = viewY = 0.0;
}
void WorldResume() {}
void WorldPause() {}
