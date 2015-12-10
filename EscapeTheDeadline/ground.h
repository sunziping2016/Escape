#pragma once
#include <windows.h>

#define ID_BORDER	0
#define ID_GROUND	2

extern double groundFriction;
extern double groundGravity;
extern COLORREF backgroundColor;

void GroundInit();
void GroundDestroy();
void GroundStart();
void GroundStop();
void GroundPause();
void GroundResume();
