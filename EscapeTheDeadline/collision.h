#pragma once

#define MAX_POINTSNUM		20
#define MAX_TYPENUM			10
#define MAX_COLLISIONNUM	10

typedef struct {
	// x, y points stored in the counter-clockwise order
	// If a ploygen, need to add an extra close point
	// Only calculated as convex ploygon
	double points[MAX_POINTSNUM][2];
	double velocity[2];
	int n, usev;
} CollisionState;

typedef struct {
	int types[MAX_TYPENUM];
	int n;
} CollisionType;

int CollisionAdd(CollisionState *(*func)(int id, int othertype, int otherid), int id,
	int type, CollisionType *othertypes,
	void(*notifier)(int id, int othertype, int otherid, double n[2], double depth, int usev));
void CollisionRemove(CollisionState *(*func)(int id, int othertype, int otherid), int id);
void CollisionProcess();
void CollisionQuery(CollisionState *points, CollisionType *othertypes,
	void(*notifier)(int othertype, int otherid, double n[2], double depth, int usev));
