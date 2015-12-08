#include <string.h>
#include <math.h>
#include "collision.h"


#define MAX_ENTITYNUM	200

static struct {
	Points *(*func)(int id);
	int id;
	int type;
	Types *othertypes;
	void(*notifier)(int id, int othertype, int otherid, double n[2], double depth);
} entities[MAX_ENTITYNUM]; // Sorted With type
static int entitiesEnd;

static void copyEntity(int to, int from)
{
	entities[to].func = entities[from].func;
	entities[to].id = entities[from].id;
	entities[to].type = entities[from].type;
	entities[to].othertypes = entities[from].othertypes;
	entities[to].notifier = entities[from].notifier;
}

int CollisionAdd(Points *(*func)(int id), int id,
	int type, Types *othertypes,
	void(*notifier)(int id, int othertype, int otherid, double n[2], double depth))
{
	int low = 0, high = entitiesEnd - 1, i;
	if (entitiesEnd == MAX_ENTITYNUM) return 1;
	while (low < high)
	{
		int i = (low + high) / 2;
		if (type > entities[i].type)
			low = i + 1;
		else
			high = i;
	}
	if (low + 1 == entitiesEnd && type > entities[low].type)
		low = entitiesEnd;
	for (i = entitiesEnd; i > low; --i)
		copyEntity(i, i - 1);
	entities[low].func = func;
	entities[low].id = id;
	entities[low].type = type;
	entities[low].othertypes = othertypes;
	entities[low].notifier = notifier;
	++entitiesEnd;
	return 0;
}

void CollisionRemove(Points *(*func)(int id), int id)
{
	int pos, i;
	for (pos = 0; pos < entitiesEnd; ++pos)
		if (entities[pos].func == func && entities[pos].id == id)
			break;
	if (pos == entitiesEnd) return;
	--entitiesEnd;
	for (i = pos; i < entitiesEnd; i++)
		copyEntity(i, i + 1);
}

#define EPSILON		1e-8

static int isCollided(Points *a, Points *b, double n[2], double *depth)
{
	double axisX, axisY, tmp, dx, dy, eX, eY;
	int side, i, s, t;
	double min[2], max[2], tmpn[2];
	int neginf[2], posinf[2], first = 1;
	Points *points[2] = { a, b };
	for (s = 0; s < 2; ++s) {
		for (side = 0; side < points[s]->n - 1; ++side) {
			axisX = points[s]->points[side][1] - points[s]->points[side + 1][1];
			axisY = points[s]->points[side + 1][0] - points[s]->points[side][0];
			tmp = sqrt(axisX * axisX + axisY * axisY);
			eX = axisX / tmp;
			eY = axisY / tmp;
			for (t = 0; t < 2; ++t) {
				neginf[t] = posinf[t] = 0;
				if (points[t]->points[0][0] == points[t]->points[points[t]->n - 1][0] &&
					points[t]->points[0][1] == points[t]->points[points[t]->n - 1][1]) continue;
				dx = points[t]->points[1][0] - points[t]->points[0][0];
				dy = points[t]->points[1][1] - points[t]->points[0][1];
				if (dx * axisX + dy *axisY > 0.0 || dx * axisX + dy * axisY == 0.0 && -dx * axisY + dy * axisX > 0.0)
					neginf[t] = 1;
				else
					posinf[t] = 1;
				dx = points[t]->points[points[t]->n - 1][0] - points[t]->points[0][0];
				dy = points[t]->points[points[t]->n - 1][1] - points[t]->points[0][1];
				if (dx * axisX + dy *axisY < 0.0 || dx * axisX + dy * axisY == 0.0 && -dx * axisY + dy * axisX > 0.0)
					neginf[t] = 1;
				else
					posinf[t] = 1;
			}
			for (t = 0; t < 2; ++t) {
				min[t] = max[t] = points[t]->points[0][0] * eX + points[t]->points[0][1] * eY;
				for (i = 1; i < points[t]->n - 1; ++i) {
					tmp = points[t]->points[i][0] * eX + points[t]->points[i][1] * eY;
					if (tmp < min[t]) min[t] = tmp;
					if (tmp > max[t]) max[t] = tmp;
				}
			}
			if ((max[0] >= min[1] || posinf[0] == 1 && posinf[1] == 0 || neginf[1] == 1 && neginf[0] == 0) &&
				(min[0] <= max[1] || posinf[1] == 1 && posinf[0] == 0 || neginf[0] == 1 && neginf[1] == 0)) {
				if ((posinf[0] == 1 || neginf[1] == 1) && (posinf[1] == 1 || neginf[0] == 1)) {
					tmp = -1;
					tmpn[0] = eX;
					tmpn[1] = eY;
				}
				else if (posinf[0] == 1 || neginf[1] == 1 ||
					     posinf[1] != 1 && neginf[0] != 1 && max[0] - min[1] > max[1] - min[0]) {
					tmp = max[1] - min[0];
					tmpn[0] = -eX;
					tmpn[1] = -eY;
				}
				else {
					tmp = max[0] - min[1];
					tmpn[0] = eX;
					tmpn[1] = eY;
				}
				if (first || tmp != -1 && (*depth == -1 || tmp < *depth)) {
					*depth = tmp;
					n[0] = tmpn[0];
					n[1] = tmpn[1];
					first = 0;
				}
			}
			else
				return 0;
		}
	}
	return 1;
}


void CollisionProcess()
{
	int i, j, s, t;
	double n[2], depth;
	for (i = 0; i < entitiesEnd; ++i) {
		for (j = i + 1; j < entitiesEnd; ++j) {
			if (entities[i].othertypes == NULL && entities[j].othertypes == NULL) continue;
			if (entities[i].othertypes != NULL)
				for (s = 0; s < entities[i].othertypes->n && entities[j].type != entities[i].othertypes->types[s]; ++s);
			if (entities[j].othertypes != NULL)
				for (t = 0; t < entities[j].othertypes->n && entities[i].type != entities[j].othertypes->types[t]; ++t);
			if ((entities[i].othertypes == NULL || s == entities[i].othertypes->n) &&
				(entities[j].othertypes == NULL || t == entities[j].othertypes->n)) continue;
			if (isCollided(entities[i].func(entities[i].id), entities[j].func(entities[j].id), n, &depth)) {
				if (entities[i].othertypes != NULL && s != entities[i].othertypes->n)
					(*entities[i].notifier)(entities[i].id, entities[j].type, entities[j].id, n, depth);
				if (entities[j].othertypes != NULL && t != entities[j].othertypes->n) {
					n[0] *= -1;
					n[1] *= -1;
					(*entities[j].notifier)(entities[j].id, entities[i].type, entities[i].id, n, depth);
				}
			}
		}
	}
}
void CollisionQuery(Points *points, Types *othertypes,
	void(*notifier)(int othertype, int otherid, double n[2], double depth))
{
	int i, s;
	double n[2], depth;
	if (othertypes == NULL || notifier == NULL) return;
	for (i = 0; i < entitiesEnd; ++i) {
		for (s = 0; s < othertypes->n && entities[i].type != othertypes->types[s]; ++s);
		if (s == othertypes->n) continue;
		if (isCollided(points, entities[i].func(entities[i].id), n, &depth))
			(*notifier)(entities[i].type, entities[i].id, n, depth);
	}
}
