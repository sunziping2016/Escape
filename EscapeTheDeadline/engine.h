#pragma once

enum GAMESTATE { NOTSTARTED, STARTED, DIED };

extern int gameState;
extern int gamePaused;

void EngineInit();
void EngineDestroy();
void EngineStart(int newState);
void EngineStop();
void EngineResume();
void EnginePause();

void EngineProcess();
