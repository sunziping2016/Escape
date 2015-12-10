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
#include "deathmenu.h"
#include "commonui.h"

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
		CommonUIStart();
		WorldStart();
		GridStart();
		GroundStart();
		PlayerStart();
		PausemenuTriggerStart();
		break;
	case DIED:
		DeathmenuStart();
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
		CommonUIStop();
		break;
	case DIED:
		DeathmenuStop();
	default:
		break;
	}
}
void EngineResume()
{
	PausemenuStop();
	CommonUIResume();
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
	CommonUIPause();
	PausemenuStart();
}

void EngineInit()
{
	StartmenuInit();
	DeathmenuInit();
	PausemenuInit();
	CommonUIInit();
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
	CommonUIDestroy();
	PausemenuDestroy();
	DeathmenuDestroy();
	StartmenuDestroy();
}

void EngineStart(int newState)
{
	StopState();
	KeyboardClear();
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
