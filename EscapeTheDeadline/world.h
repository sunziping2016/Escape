#pragma once
#include <windows.h>
#include <math.h>

// Create a virtual coordinate system regardless of the real windows size.
// And user can set tracked object when it showed always be shown in the window

#define ROUND(pos)		((int)lround(pos))

extern double viewX, viewY;
extern double viewRect[4];

void WorldInit();
void WorldDestroy();
void WorldStart();
void WorldStop();
void WorldPause();
void WorldResume();

int WorldX(double x);
int WorldY(double y);
void WorldSetViewport(double x, double y);
void WorldSetTracked(void(*func)(int id, double *x, double *y), int id);
