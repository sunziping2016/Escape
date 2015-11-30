#pragma once

enum GAMESTATE { NOTSTARTED, STARTED, PAUSED, DIED };

extern int gameState;

void EngineInit();
void EngineDestroy();
void EngineStart(int newState);
void EngineStop();

void EngineProcess();
