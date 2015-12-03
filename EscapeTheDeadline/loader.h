#pragma once
#include <Windows.h>
#include <tchar.h>

int LoaderLoad(TCHAR *filename);

void LoaderInit();
void LoaderDestroy();
int LoaderReload();
int LoaderAdd(TCHAR *commandName, int(*commandFunc)(TCHAR *command));
