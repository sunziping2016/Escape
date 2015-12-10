#pragma warning(disable: 4996)
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "timer.h"
#include "drawer.h"
#include "engine.h"
#include "loader.h"

#define MARGIN				16

#define COLOR_ERROR			RGB(0xff, 0x00, 0x00)
#define FONTSIZE_ERROR		16
#define FONTNAME_ERROR		TEXT("SimSun")

#define ERROR_TIME			1500
#define ERROR_BUFFERSIZE	200

static TCHAR errorBuffer[ERROR_BUFFERSIZE];
static int errorTimerID;
static HFONT hFontError;
static void ErrorDrawer(int id, HDC hDC)
{
	int len;
	len = (int)wcslen(errorBuffer);
	if (len == 0) return;
	SelectObject(hDC, hFontError);
	SetTextColor(hDC, COLOR_ERROR);
	TextOut(hDC, MARGIN, DrawerY - MARGIN - FONTSIZE_ERROR, errorBuffer, len);
}
static void ErrorTimer(int id, int ms)
{
	if (id != errorTimerID) return;
	errorBuffer[0] = L'\0';
}
int ErrorPrintf(wchar_t *format, ...)
{
	int ret;
	va_list args;
	va_start(args, format);
	ret = vswprintf(errorBuffer, format, args);
	va_end(args);
	++errorTimerID;
	TimerAdd(ErrorTimer, errorTimerID, ERROR_TIME);
	return ret;
}

void CommonUIInit()
{
	hFontError = CreateFont(FONTSIZE_ERROR, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_ERROR);
	DrawerAdd(ErrorDrawer, 0, -15);
}
void CommonUIDestroy()
{
	DeleteObject(hFontError);
}
void CommonUIStart()
{

}
void CommonUIStop() {}
void CommonUIResume() {}
void CommonUIPause(){}
