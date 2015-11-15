#pragma once

#define MAX_POINTSNUM	20
#define MAX_TYPENUM		10

typedef struct {
	// x, y points stored in the counter-clockwise order
	// If a ploygen, need to add an extra close point
	// Only calculated as convex ploygon
	double points[MAX_POINTSNUM][2];
	int n;
	//double rect[4]; // For optimization, user may ignore it
} Points;

typedef struct {
	int types[MAX_TYPENUM];
	int n;
} Types;

void CollisionInit();
void CollisionDestroy();

int CollisionAdd(Points *(*func)(int id), int id,
	int type, Types *othertypes,
	void(*notifier)(int id, int othertype, int otherid, double n[2], double depth));
void CollisionRemove(Points *(*func)(int id), int id);
void CollisionProcess();
