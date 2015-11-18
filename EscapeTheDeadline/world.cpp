#include "drawer.h"
#include "timer.h"
#include "world.h"

#define MAX_WORLDDRAWERNUM 200

double WorldX, WorldY;


static int viewX, viewY;
static int viewVX, viewVY;

void WorldInit(int x, int y)
{
	WorldX = x;
	WorldY = y;
	viewX = x / 2;
	viewY = y / 2;
}

void WorldDestroy() {}

static void(*trackedFunc)(int id, int *x, int *y);
static int trackedID;

void WorldSetTracked(void(*func)(int id, int *x, int *y), int id)
{
	trackedFunc = func;
	trackedID = id;
}

