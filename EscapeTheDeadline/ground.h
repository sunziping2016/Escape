#pragma once
#define ID_BORDER	0

extern double groundFriction;
extern double groundGravity;

void GroundInit();
void GroundDestroy();
void GroundStart();
void GroundStop();
void GroundPause();
void GroundResume();
