#pragma once
int LogInit();
void LogDestroy();

int LogPrintf(const char *format, ...); // C99
