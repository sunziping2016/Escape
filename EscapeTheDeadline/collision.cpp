#include <string.h>
#include "collision.h"

#define MAX_ENTITYNUM	200

static struct {
	Points *(*func)(int id);
	int id;
	int type;
	Types *othertypes;
	void(*notifier)(int id, int othertype, int otherid);
} entities[MAX_ENTITYNUM]; // Sorted With type
static int entitiesEnd;

void CollisionInit() {}
void CollisionDestroy() {}

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
	void(*notifier)(int id, int othertype, int otherid))
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
	if (low + 1 == entitiesEnd && type < entities[low].type)
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

static int isCollided(int i, int j)
{
	// TODO
}


void CollisionProcess()
{
	int entitiesIndex[MAX_ENTITYNUM], nIndex = 0;
	int i, j, k, l;
	memset(entitiesIndex, 0, sizeof(entities));
	// Build an index for better search performance
	for (i = 0; i < entitiesEnd; ++i)
		if (i == 0 || entities[i - 1].type != entities[i].type)
			entitiesIndex[entities[i].type] = i;
	for (i = 0; i < entitiesEnd; ++i) {
		for (k = 0; k < entities[i].othertypes->n; ++k) {
			j = entitiesIndex[entities[i].othertypes->types[k]];
			if (j < i) j = i;
			for (; entities[j].type != entities[i].othertypes->types[k]; ++j) {
				// Now entities[i] and entities[j] is to be tested whether collided
				if (isCollided(i, j)) {
					(*entities[i].notifier)(entities[i].id, entities[j].type, entities[j].id);
					for (l = 0; l < entities[j].othertypes->n && entities[i].type != entities[j].othertypes->types[l]; ++l);
					if(l != entities[j].othertypes->n)
						(*entities[j].notifier)(entities[j].id, entities[i].type, entities[i].id);
				}
			}
		}
	}
}
