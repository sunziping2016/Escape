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
#include "generator.h"
#include "bonus.h"

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
		GeneratorStart();
		GridStart();
		GroundStart();
		PlayerStart();
		BonusStart();
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
		BonusStop();
		PlayerStop();
		GroundStop();
		GridStop();
		GeneratorStop();
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
	BonusResume();
	gamePaused = 0;
}
void EnginePause()
{
	gamePaused = 1;
	BonusPause();
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
	GeneratorInit();
	GridInit();
	GroundInit();
	PlayerInit();
	BonusInit();
	StartState(NOTSTARTED);
	gamePaused = 0;
}
void EngineDestroy()
{
	BonusDestroy();
	PlayerDestroy();
	GroundDestroy();
	GridDestroy();
	GeneratorDestroy();
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
	WorldProcess();
	CollisionProcess();
}
