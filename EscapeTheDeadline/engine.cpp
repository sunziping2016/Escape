#include "engine.h"
#include "collision.h"

#include "test.h"

void EngineInit()
{
	CollisionInit();
	TestInit();
}

void EngineDestroy() 
{
	TestDestroy();
	CollisionDestroy();
}

void EngineProcess()
{
	CollisionProcess();
}
