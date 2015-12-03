#include "engine.h"
#include "keyboard.h"
#include "timer.h"
#include "collision.h"
#include "world.h"
#include "player.h"
#include "startmenu.h"
#include "pausemenu.h"

#include "test.h"

int gameState;
int gamePaused;

static void StartState(int newState)
{
	gameState = newState;
	switch (newState)
	{
	case NOTSTARTED:
		StartmenuStart();
		break;
	case STARTED:
		WorldStart();
		TestStart();
		PlayerStart();
		break;
	default:
		break;
	}
}

static void StopState()
{
	switch (gameState)
	{
	case NOTSTARTED:
		StartmenuStop();
		break;
	case STARTED:
		if (gamePaused)
			EngineResume();
		PlayerStop();
		TestStop();
		WorldStop();
		break;
	default:
		break;
	}
}
void EngineResume()
{
	PausemenuStop();
	WorldResume();
	TestResume();
	PlayerResume();
	gamePaused = 0;
}
void EnginePause()
{
	gamePaused = 1;
	PlayerPause();
	TestPause();
	WorldPause();
	PausemenuStart();
}

void EngineInit()
{
	StartmenuInit();
	PausemenuInit();
	WorldInit();
	TestInit();
	PlayerInit();
	StartState(NOTSTARTED);
	gamePaused = 0;
}
void EngineDestroy()
{
	PlayerDestroy();
	TestDestroy();
	WorldDestroy();
	PausemenuDestroy();
	StartmenuDestroy();
}

void EngineStart(int newState)
{
	StopState();
	KeyboardClear();
	TimerClear();
	StartState(newState);
}
void EngineStop()
{
	StopState();
	PostQuitMessage(0);
}

void EngineProcess()
{
	CollisionProcess();
}
