#pragma once
#define ID_BORDER	0
#include <windows.h>
extern double groundFriction;
extern double groundGravity;
extern COLORREF groundColor;

void GroundInit();
void GroundDestroy();
void GroundStart();
void GroundStop();
void GroundPause();
void GroundResume();
