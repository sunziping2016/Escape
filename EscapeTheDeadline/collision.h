#pragma once

#define MAX_POINTSNUM	20
#define MAX_TYPENUM		10

typedef struct {
	double points[MAX_POINTSNUM][2]; // x, y
	int n;
	double rect[4]; // For optimization, user may ignore it
} Points;

typedef struct {
	int types[MAX_TYPENUM];
	int n;
} Types;

void CollisionInit();
void CollisionDestroy();

int CollisionAdd(Points *(*func)(int id), int id,
	int type, Types *othertypes,
	void(*notifier)(int id, int othertype, int otherid));
void CollisionRemove(Points *(*func)(int id), int id);
void CollisionProcess();
