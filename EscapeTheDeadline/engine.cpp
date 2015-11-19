#include "engine.h"
#include "collision.h"
#include "world.h"

#include "test.h"

void EngineInit()
{
	CollisionInit();
	WorldInit();
	TestInit();
}

void EngineDestroy() 
{
	TestDestroy();
	WorldDestroy();
	CollisionDestroy();
}

void EngineProcess()
{
	CollisionProcess();
}
