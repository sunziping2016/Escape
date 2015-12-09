#pragma once
#define ID_PLAYER		1
#define PLAYERLIFE_MAX	100.0

void PlayerInit();
void PlayerDestroy();
void PlayerStart();
void PlayerStop();
void PlayerPause();
void PlayerResume();

void PlayerAddScore(int num);
void PlayerDie();
void PlayerAddLife(int num);
void PlayerMinusLife(int num);
