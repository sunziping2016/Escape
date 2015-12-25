#pragma once
#include <windows.h>
#include "collision.h"

#define ID_BORDER	0
#define ID_GROUND	2

extern double groundFriction;
extern double groundGravity;
extern COLORREF backgroundColor;

#define MAX_GROUNDLEN	100
typedef struct {
	double fromX, fromY;
	double toX, toY;
	double color;
	int leftId, rightId;
	int traits;
	int invisible;
	int hasTimer;
	CollisionState points;
} GroundType;
extern GroundType grounds[MAX_GROUNDLEN];
extern int groundsEnd;
extern int groundsMostId[2];
extern int groundsLowestID;
int GroundAdd(double fromX, double fromY, double toX, double toY, int traits, int invisible);
void GroundRemove(int id);
void GroundClear();

void GroundInit();
void GroundDestroy();
void GroundStart();
void GroundStop();
void GroundPause();
void GroundResume();
