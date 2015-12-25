#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include "player.h"
#include "drawer.h"
#include "generator.h"
#include "collision.h"
#include "ground.h"
#include "world.h"
#include "commonui.h"
#include "loader.h"
#include "engine.h"
#include "bonus.h"

#define GENERATE_DELTA		-50
#define CONSUME_DELTA		600
#define LOWEST_DELTA		300

int Probability(double r)
{
	if (rand() <= r * RAND_MAX)
		return 1;
	else
		return 0;
}

double AverageRandom(double min, double max)
{
	return (double)rand() / RAND_MAX*(max - min) + min;
}
#define PI		3.1415926535
double Normal(double x, double miu, double sigma)
{
	return 1.0 / sqrt(2 * PI * sigma) * exp(-1 * (x - miu) * (x - miu) / (2 * sigma * sigma));
}
double NormalRandom(double miu, double sigma, double min, double max)
{
	double x, y, dScope;
	do
	{
		x = AverageRandom(min, max);
		y = Normal(x, miu, sigma);
		dScope = AverageRandom(0.0, Normal(miu, miu, sigma));
	} while (dScope > y);
	return x;
}


static CollisionState *GenerateCollision(int id, int othertype, int otherid)
{
	static CollisionState cs;
	if (id == 0) {
		cs.points[0][0] = viewRect[0] - GENERATE_DELTA;
		cs.points[0][1] = 1.0;
		cs.points[1][0] = viewRect[0] - GENERATE_DELTA;
		cs.points[1][1] = -1.0;
	}
	else {
		cs.points[0][0] = viewRect[2] + GENERATE_DELTA;
		cs.points[0][1] = -1.0;
		cs.points[1][0] = viewRect[2] + GENERATE_DELTA;
		cs.points[1][1] = 1.0;
	}
	cs.n = 2;
	cs.usev = 0;
	return &cs;
}
static CollisionState *ConsumeCollision(int id, int othertype, int otherid)
{
	static CollisionState cs;
	if (id == 0) {
		cs.points[0][0] = viewRect[0] - CONSUME_DELTA;
		cs.points[0][1] = -1.0;
		cs.points[1][0] = viewRect[0] - CONSUME_DELTA;
		cs.points[1][1] = 1.0;
	}
	else {
		cs.points[0][0] = viewRect[2] + CONSUME_DELTA;
		cs.points[0][1] = 1.0;
		cs.points[1][0] = viewRect[2] + CONSUME_DELTA;
		cs.points[1][1] = -1.0;
	}
	cs.n = 2;
	cs.usev = 0;
	return &cs;
}

static int hasGeneratorGround; 
static CollisionType groundMostCollisionType = { { ID_GENERATE, ID_CONSUME },  2 };
static CollisionType groundLowestCollisionType = { { ID_PLAYER },  1 };


static void GroundMostCollisionNotifier(int id, int othertype, int otherid, double n[2], double depth, int usev)
 {
	if (othertype == ID_GENERATE) {
		if (id == 1 && otherid == 1) {
			// Right Generate
			double fromX, fromY, toX, toY;
			int traits, invisible;
			if (grounds[groundsMostId[1]].traits == 1 || Probability(0.75))
				fromX = grounds[groundsMostId[1]].toX;
			else
				fromX = grounds[groundsMostId[1]].toX + 150;
			if (Probability(0.9))
				fromY = grounds[groundsMostId[1]].toY;
			else if (Probability(0.5))
				fromY = grounds[groundsMostId[1]].toY + NormalRandom(50.0, 10.0, 30.0, 70.0);
			else
				fromY = grounds[groundsMostId[1]].toY - NormalRandom(50.0, 10.0, 30.0, 70.0);
			toX = fromX + NormalRandom(200.0, 30.0, 120.0, 280.0);
			if (Probability(0.8))
				toY = fromY;
			else if (Probability(0.5))
				toY = fromY + NormalRandom(50.0, 10.0, 30.0, 70.0);
			else
				toY = fromY - NormalRandom(50.0, 10.0, 30.0, 70.0);
			traits = invisible = 0;
			if (Probability(0.02))
				invisible = 1;
			else if (fromX == grounds[groundsMostId[1]].toX && grounds[groundsMostId[1]].traits != 1 && Probability(0.1))
				traits = 1;
			GroundAdd(fromX, fromY, toX, toY, traits, invisible);
			if (Probability(0.4)) {
				if (Probability(0.6))
					BonusAdd((fromX + toX) / 2, (fromY + toY) / 2 - NormalRandom(100.0, 20.0, 50.0, 150.0), 1);
				else
					BonusAdd((fromX + toX) / 2, (fromY + toY) / 2 - NormalRandom(100.0, 20.0, 50.0, 150.0), 0);
			}
		}
	}
	else {
		if (id == 0 && otherid == 0)
			GroundRemove(groundsMostId[0]);
	}
}
static CollisionState *GroundMostCollision(int id, int othertype, int otherid)
{
	static CollisionState cs;
	if (groundsMostId[id] == -1) {
		cs.n = 0;
		cs.usev = 0;
		return &cs;
	}
	if (id == 0)
		cs.points[0][0] = grounds[groundsMostId[0]].fromX;
	else
		cs.points[0][0] = grounds[groundsMostId[1]].toX;
	cs.points[0][1] = 1.0;
	cs.points[1][0] = cs.points[0][0];
	cs.points[1][1] = -1.0;
	cs.points[2][0] = cs.points[0][0];
	cs.points[2][1] = 1.0;
	cs.n = 3;
	cs.usev = 0;
	return &cs;
}
static void GroundLowestCollisionNotifier(int id, int othertype, int otherid, double n[2], double depth, int usev)
{
	if (othertype == ID_PLAYER)
		PlayerDie();
}
static CollisionState *GroundLowestCollision(int id, int othertype, int otherid)
{
	static CollisionState cs;
	double lowest;
	if(groundsLowestID == -1) {
		cs.n = 0;
		cs.usev = 0;
		return &cs;
	}
	lowest = max(grounds[groundsLowestID].fromY, grounds[groundsLowestID].toY);
	cs.points[0][0] = -1.0;
	cs.points[0][1] = lowest + LOWEST_DELTA;
	cs.points[1][0] = 1.0;
	cs.points[1][1] = lowest + LOWEST_DELTA;
	cs.n = 2;
	cs.usev = 0;
	return &cs;
}
void GeneratorGroundStart()
{
	CollisionAdd(GenerateCollision, 0, ID_GENERATE, NULL, NULL);
	CollisionAdd(GenerateCollision, 1, ID_GENERATE, NULL, NULL);
	CollisionAdd(ConsumeCollision, 0, ID_CONSUME, NULL, NULL);
	CollisionAdd(ConsumeCollision, 1, ID_CONSUME, NULL, NULL);
	CollisionAdd(GroundMostCollision, 0, ID_GROUNDMOST, &groundMostCollisionType, GroundMostCollisionNotifier);
	CollisionAdd(GroundMostCollision, 1, ID_GROUNDMOST, &groundMostCollisionType, GroundMostCollisionNotifier);
	CollisionAdd(GroundLowestCollision, 0, ID_GROUNDLOWEST, &groundLowestCollisionType, GroundLowestCollisionNotifier);
}
void GeneratorGroundStop()
{
	CollisionRemove(GenerateCollision, 0);
	CollisionRemove(GenerateCollision, 1);
	CollisionRemove(ConsumeCollision, 0);
	CollisionRemove(ConsumeCollision, 1);
	CollisionRemove(GroundMostCollision, 0);
	CollisionRemove(GroundMostCollision, 1);
	CollisionRemove(GroundLowestCollision, 0);
}
int GeneratorCreate(wchar_t *command)
{
	int newhasGeneratorGround;
	if (swscanf(command, L"%*s%d", &newhasGeneratorGround) == 1 && (newhasGeneratorGround == 0 || newhasGeneratorGround == 1)) {
		if (newhasGeneratorGround == hasGeneratorGround) return 0;
		hasGeneratorGround = newhasGeneratorGround;
		if (gameState == STARTED) {
			if (hasGeneratorGround == 1) GeneratorGroundStart();
			else GeneratorGroundStop();
		}
		return 0;
	}
	return 1;
}
void GeneratorInit()
{
	srand((unsigned int)time(NULL));
	LoaderAdd(L"generator", GeneratorCreate);
}
void GeneratorDestroy() {}
void GeneratorStart()
{
	if (hasGeneratorGround == 1)
		GeneratorGroundStart();
}
void GeneratorStop()
{
	if (hasGeneratorGround) {
		hasGeneratorGround = 0;
		GeneratorGroundStop();
	}
}
