#include "engine.h"
#include "keyboard.h"
#include "collision.h"
#include "world.h"
#include "player.h"
#include "startmenu.h"

#include "test.h"

int gameState;

void EngineInit()
{
	WorldInit();
	//PlayerInit();
	StartmenuInit();
	TestInit();
	gameState = NOTSTARTED;
	StartmenuStart();
}

void EngineDestroy() 
{
	TestDestroy();
	StartmenuDestroy();
	//PlayerDestory();
	WorldDestroy();
}

static void StopCurrentState()
{
	switch (NOTSTARTED)
	{
	case NOTSTARTED:
		StartmenuStop();
		break;
	case STARTED:
		TestStop();
		WorldStop();
		break;
	default:
		break;
	}
}

void EngineStart(int newState)
{
	StopCurrentState();
	KeyboardClear();
	switch (newState)
	{
	case NOTSTARTED:
		StartmenuStart();
		break;
	case STARTED:
		TestStart();
		WorldStart();
		break;
	default:
		break;
	}
	gameState = newState;
}

void EngineStop()
{
	StopCurrentState();
	PostQuitMessage(0);
}

void EngineProcess()
{
	CollisionProcess();
}
