#include "engine.h"
#include "keyboard.h"
#include "timer.h"
#include "collision.h"
#include "world.h"
#include "player.h"
#include "startmenu.h"
#include "pausemenu.h"
#include "ground.h"
#include "grid.h"

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
		GridStart();
		GroundStart();
		PlayerStart();
		PausemenuTriggerStart();
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
		PausemenuTriggerStop();
		PlayerStop();
		GroundStop();
		GridStop();
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
	GroundResume();
	PlayerResume();
	gamePaused = 0;
}
void EnginePause()
{
	gamePaused = 1;
	PlayerPause();
	GroundPause();
	WorldPause();
	PausemenuStart();
}

void EngineInit()
{
	StartmenuInit();
	PausemenuInit();
	WorldInit();
	GridInit();
	GroundInit();
	PlayerInit();
	StartState(NOTSTARTED);
	gamePaused = 0;
}
void EngineDestroy()
{
	PlayerDestroy();
	GroundDestroy();
	GridDestroy();
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
	if (gameState != STARTED || gamePaused) return;
	CollisionProcess();
}
