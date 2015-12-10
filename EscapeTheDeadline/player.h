#pragma once
#define ID_PLAYER		1
#define PLAYERLIFE_MAX	100.0

extern int playerIsDied;

void PlayerInit();
void PlayerDestroy();
void PlayerStart();
void PlayerStop();
void PlayerPause();
void PlayerResume();

void PlayerAddScore(int num);
int PlayerGetScore();
void PlayerDie();
void PlayerAddLife(double num);
void PlayerMinusLife(double num);
