#pragma warning(disable: 4996)
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "timer.h"
#include "drawer.h"
#include "engine.h"
#include "loader.h"
#include "keyboard.h"

#define MARGIN				16

#define COLOR_ERROR			RGB(0xff, 0x00, 0x00)
#define COLOR_CMD			RGB(0x00, 0x00, 0x00)
#define FONTSIZE_ERROR		16
#define FONTSIZE_CMD		16
#define FONTNAME_ERROR		TEXT("SimSun")
#define FONTNAME_CMD		TEXT("SimSun")

#define ERROR_TIME			1500
#define ERROR_BUFFERSIZE	200

static TCHAR errorBuffer[ERROR_BUFFERSIZE];
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
	if (id < 0) return;
	errorBuffer[0] = L'\0';
}
static int ErrorTimerUpdateID(int id) { return -1; }
int ErrorPrintf(wchar_t *format, ...)
{
	int ret;
	va_list args;
	va_start(args, format);
	ret = vswprintf(errorBuffer, format, args);
	va_end(args);
	TimerUpdateID(ErrorTimer, ErrorTimerUpdateID);
	TimerAdd(ErrorTimer, 0, ERROR_TIME);
	return ret;
}

#define MAX_COMMANDLINEBUFFER 200
static TCHAR commandLine[MAX_COMMANDLINEBUFFER + 1];
static int commandLinePos;
int commandLineFocus;
static HFONT hFontCommandLine;

static void CommandLineSetpos()
{
	HDC hDC = GetDC(g_hWnd);
	SIZE size;
	SelectObject(hDC, hFontCommandLine);
	GetTextExtentPoint(hDC, commandLine, commandLinePos, &size);
	SetCaretPos(MARGIN + size.cx, DrawerY - MARGIN - FONTSIZE_ERROR - FONTSIZE_CMD);
	ReleaseDC(g_hWnd, hDC);
}
static void CommandLineDrawer(int id, HDC hDC)
{
	int len;
	len = (int)wcslen(commandLine);
	if (len == 0) return;
	SelectObject(hDC, hFontCommandLine);
	SetTextColor(hDC, COLOR_CMD);
	TextOut(hDC, MARGIN, DrawerY - MARGIN - FONTSIZE_ERROR - FONTSIZE_CMD, commandLine, len);
	CommandLineSetpos();
}
static void CommandLineKey(int id, WPARAM wParam, int keytype)
{
	TCHAR code = (TCHAR)wParam;
	int len = (int)wcslen(commandLine);
	if (keytype == KEYCHAR) {
		if (!commandLineFocus && code == L':') {
			CreateCaret(g_hWnd, (HBITMAP)NULL, 1, FONTSIZE_CMD);
			commandLineFocus = 1;
			commandLine[commandLinePos++] = L':';
			commandLine[commandLinePos] = L'\0';
			CommandLineSetpos();
			ShowCaret(g_hWnd);
		}
		else if (commandLineFocus) {
			if (code == L'\r' || code == VK_ESCAPE) {
				if(code == L'\r' && LoaderRun(commandLine + 1))
					ErrorPrintf(L"CommandLineError: Cannot parse the command");
				commandLine[0] = L'\0';
				commandLinePos = 0;
				commandLineFocus = 0;
				HideCaret(g_hWnd);
				DestroyCaret();
				memset(KeyboardIsDown, 0, sizeof(KeyboardIsDown));
			}
			if (code < L' ' || len == MAX_COMMANDLINEBUFFER) return;
			MoveMemory(commandLine + commandLinePos + 1, commandLine + commandLinePos, (len - commandLinePos + 1) * sizeof(TCHAR));
			commandLine[commandLinePos++] = code;
			CommandLineSetpos();
		}
	}
	else if (keytype == KEYDOWN && commandLineFocus) {
		switch (code)
		{
		case VK_LEFT:
			if (commandLinePos > 1) --commandLinePos;
			CommandLineSetpos();
			break;
		case VK_RIGHT:
			if (commandLinePos < len) ++commandLinePos;
			CommandLineSetpos();
			break;
		case VK_HOME:
			commandLinePos = 1;
			break;
		case VK_END:
			commandLinePos = len;
			break;
		case VK_BACK:
			if (commandLinePos > 1) {
				MoveMemory(commandLine + commandLinePos - 1, commandLine + commandLinePos, (len - commandLinePos + 1) * sizeof(TCHAR));
				commandLinePos--;
			}
			CommandLineSetpos();
			break;
		}
	}
}
void CommonUIInit()
{
	hFontError = CreateFont(FONTSIZE_ERROR, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_ERROR);
	hFontCommandLine = CreateFont(FONTSIZE_CMD, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_CMD);
	DrawerAdd(ErrorDrawer, 0, -15);
	DrawerAdd(CommandLineDrawer, 0, -15);
	KeyboardAdd(CommandLineKey, 0);
}
void CommonUIDestroy()
{
	DeleteObject(hFontCommandLine);
	DeleteObject(hFontError);
}
void CommonUIStart()
{

}
void CommonUIStop() {}
void CommonUIResume() {}
void CommonUIPause(){}
