#pragma once
#include <windows.h>

// Create a virtual coordinate system regardless of the real windows size.
// And user can set tracked object when it showed always be shown in the window

void WorldInit();
void WorldDestroy();
void WorldStart();
void WorldStop();
void WorldPause();
void WorldResume();

void WorldMapper(double x, double y, int *newx, int *newy);
void WorldSetTracked(void(*func)(int id, double *x, double *y), int id);

POINT WorldSetMapper(HDC hDC, double x, double y);
void WorldResetMapper(HDC hDC, POINT *orign);
