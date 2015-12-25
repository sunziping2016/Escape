#pragma once
// Error
extern int commandLineFocus;
int ErrorPrintf(wchar_t *format, ...);

void CommonUIInit();
void CommonUIDestroy();
void CommonUIStart();
void CommonUIStop();
void CommonUIResume();
void CommonUIPause();
