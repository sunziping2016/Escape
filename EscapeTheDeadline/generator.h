#pragma once
#define ID_GENERATE		4
#define ID_CONSUME		5
#define ID_GROUNDMOST	6
#define ID_GROUNDLOWEST 7

int Probability(double r);

void GeneratorInit();
void GeneratorDestroy();
void GeneratorStart();
void GeneratorStop();
