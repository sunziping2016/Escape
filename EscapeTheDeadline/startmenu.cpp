#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <math.h>
#include "startmenu.h"

#include "timer.h"
#include "drawer.h"
#include "keyboard.h"
#include "engine.h"
#include "loader.h"

#define GAMEFILE_MAXLEN			50

#define GAME_TITLE				TEXT("Escape the Deadline")
#define GAME_PROMPT				TEXT("Choose level: ")
#define GAME_DESCRIPITION		TEXT("Press SPACE to start the game.")
#define GAME_COPYRIGHT			TEXT("Copyright (c) By Sun Ziping. Version 0.1.0. All rights reserved.")

#define FONTNAME				TEXT("GothicE")
#define FONTNAME_SANS			TEXT("Monotype Corsiva")

#define FONTSIZE_ITEM			64
#define FONTSIZE_TITLE			96
#define FONTSIZE_DESCRIPITION	48
#define FONTSIZE_COPYRIGHT		32
#define MARGIN					24

#define COLOR_TITLE				RGB(0xb2, 0x22, 0x22)
#define COLOR_ITEM				RGB(0x56, 0x3d, 0x7c)
#define COLOR_DESCRIPITION		RGB(0x00, 0x80, 0x80)
#define COLOR_PROMPT			RGB(0x80, 0x80, 0x00)
#define COLOR_COPYRIGHT			RGB(0xd3, 0xd3, 0xd3)
#define COLOR_BK				RGB(0xff, 0xf8, 0xdc)
#define COLOR_DARK				RGB(0x00, 0x00, 0x00)

#define MAXDARKING				10

#define GAMEFILE_DIR			TEXT("games//")
#define GAMEFILE_EXTENSION		TEXT(".txt")

static TCHAR gamefiles[GAMEFILE_MAXLEN][MAX_PATH];
static int gamefilesEnd = 0;

static HFONT hMenuFont, hMenuSelectedFont, hTitleFont, hDescriptionFont, hCopyrightFont;
static SIZE itemSize;
static int selected;
static double nowpos, nowvelocity;
static int darking;
static enum {START, READY, END} menuState;

static void GamefilesInit()
{
	int i;
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(GAMEFILE_DIR TEXT("*") GAMEFILE_EXTENSION, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) return;
	do {
		wcscpy_s(gamefiles[gamefilesEnd], findFileData.cFileName);
		for (i = 0; gamefiles[gamefilesEnd][i] != TEXT('\0') && gamefiles[gamefilesEnd][i] != TEXT('.'); ++i);
		gamefiles[gamefilesEnd][i] = TEXT('\0');
		++gamefilesEnd;
	} while (FindNextFile(hFind, &findFileData));
}

#define PI 3.1415926535

static void StartmenuDrawer(int id, HDC hDC)
{
	HBRUSH hBkColor;
	static int len[GAMEFILE_MAXLEN], titleLen, descripitionLen, promptLen, copyrightLen;
	static SIZE size[GAMEFILE_MAXLEN], titleSize, descripitionSize, promptSize, copyrightSize;
	static int first = 1;
	int i;
	double distance, r1, r2, num;
	RECT rect = { 0, 0, DrawerX, DrawerY };
	if (gameState != NOTSTARTED) return;
	if (first) {
		SelectObject(hDC, hMenuSelectedFont);
		for (i = 0; i < gamefilesEnd; ++i) {
			len[i] = (int)wcslen(gamefiles[i]);
			GetTextExtentPoint32(hDC, gamefiles[i], len[i], &size[i]);
			if (size[i].cx > itemSize.cx) itemSize.cx = size[i].cx;
			if (size[i].cy > itemSize.cy) itemSize.cy = size[i].cy;
		}
		promptLen = (int)wcslen(GAME_PROMPT);
		GetTextExtentPoint32(hDC, GAME_PROMPT, promptLen, &promptSize);
		SelectObject(hDC, hTitleFont);
		titleLen = (int)wcslen(GAME_TITLE);
		GetTextExtentPoint32(hDC, GAME_TITLE, titleLen, &titleSize);
		SelectObject(hDC, hDescriptionFont);
		descripitionLen = (int)wcslen(GAME_DESCRIPITION);
		GetTextExtentPoint32(hDC, GAME_DESCRIPITION, descripitionLen, &descripitionSize);
		SelectObject(hDC, hCopyrightFont);
		copyrightLen = (int)wcslen(GAME_COPYRIGHT);
		GetTextExtentPoint32(hDC, GAME_COPYRIGHT, copyrightLen, &copyrightSize);
		first = 0;
	}
	r1 = sin((double)darking / MAXDARKING * PI / 2.0);
	hBkColor = CreateSolidBrush(COLOR_BK);
	FillRect(hDC, &rect, hBkColor);
	DeleteObject(hBkColor);
	SetBkMode(hDC, TRANSPARENT);
	num = (double)(DrawerY - 2 * MARGIN - FONTSIZE_DESCRIPITION - FONTSIZE_TITLE - FONTSIZE_DESCRIPITION) / FONTSIZE_ITEM / 2.0 - 0.5;
	for (i = 0; i < gamefilesEnd; ++i) {
		distance = fabs((double)i * itemSize.cy - nowpos + 0.5);
		if (distance >= num * itemSize.cy) continue;
		r2 = distance / (num * itemSize.cy);
		SetTextColor(hDC, DrawerColor(COLOR_BK, COLOR_ITEM, r2));
		if (i == selected) SelectObject(hDC, hMenuSelectedFont);
		else SelectObject(hDC, hMenuFont);
		TextOut(hDC, (DrawerX - itemSize.cx + promptSize.cx) / 2, (DrawerY - itemSize.cy) / 2 - (int)(nowpos + 0.5) + itemSize.cy * i, gamefiles[i], len[i]);
	}
	SetTextColor(hDC, COLOR_PROMPT);
	SelectObject(hDC, hMenuFont);
	TextOut(hDC, (DrawerX - itemSize.cx - promptSize.cx) / 2, (DrawerY - promptSize.cy) / 2, GAME_PROMPT, promptLen);
	SetTextColor(hDC, COLOR_TITLE);
	SelectObject(hDC, hTitleFont);
	TextOut(hDC, (DrawerX - titleSize.cx) / 2, MARGIN, GAME_TITLE, titleLen);
	SetTextColor(hDC, COLOR_DESCRIPITION);
	SelectObject(hDC, hDescriptionFont);
	TextOut(hDC, (DrawerX - descripitionSize.cx) / 2, DrawerY - MARGIN - FONTSIZE_DESCRIPITION - FONTSIZE_COPYRIGHT, GAME_DESCRIPITION, descripitionLen);
	SetTextColor(hDC, COLOR_COPYRIGHT);
	SelectObject(hDC, hCopyrightFont);
	TextOut(hDC, DrawerX - MARGIN - copyrightSize.cx, DrawerY - MARGIN - FONTSIZE_COPYRIGHT, GAME_COPYRIGHT, copyrightLen);
	if(darking)
		DrawerAlphaColor(hDC, 0, 0, DrawerX, DrawerY, COLOR_DARK, r1);
}

#define factor1 0.05
#define factor2 (2 * sqrt(factor1))

static void StartmenuTimer(int id, int ms)
{
	TCHAR filename[MAX_PATH];
	double distance;
	if (gameState != NOTSTARTED || gamefilesEnd == 0) return;
	if (menuState == END) {
		if (darking >= MAXDARKING)
			EngineStart(STARTED);
		else
			++darking;
	}
	else if (menuState == START) {
		if (darking == 0)
			menuState = READY;
		else
			--darking;
	}
	else if (KeyboardIsDown[VK_ESCAPE])
		EngineStop();
	else if (KeyboardIsDown[VK_SPACE] || KeyboardIsDown[VK_RETURN] || KeyboardIsDown[VK_RIGHT]) {
		wcscpy(filename, GAMEFILE_DIR);
		wcscat(filename, gamefiles[selected]);
		wcscat(filename, GAMEFILE_EXTENSION);
		LoaderLoad(filename);
		menuState = END;
	}
	else {
		selected = (selected + KeyboardGetNum[VK_DOWN] - KeyboardGetNum[VK_UP]
			+ 3 * KeyboardGetNum[VK_PRIOR] - 3 * KeyboardGetNum[VK_NEXT] + 5 * gamefilesEnd) % gamefilesEnd;
		if (KeyboardIsDown[VK_HOME]) selected = 0;
		else if (KeyboardIsDown[VK_END]) selected = gamefilesEnd - 1;
		distance = (double)selected * itemSize.cy - nowpos;
		nowvelocity += factor1 * distance - factor2 * nowvelocity;
		nowpos += nowvelocity;
	}
	TimerAdd(StartmenuTimer, id, ms + 20);
}

void StartmenuInit()
{
	GamefilesInit();
	AddFontResource(FONTNAME TEXT(".ttf"));
	hMenuFont = CreateFont(FONTSIZE_ITEM, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME);
	hMenuSelectedFont = CreateFont(FONTSIZE_ITEM, 0, 0, 0, 0, FALSE, TRUE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME);
	hTitleFont = CreateFont(FONTSIZE_TITLE, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME);
	hDescriptionFont = CreateFont(FONTSIZE_DESCRIPITION, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_SANS);
	hCopyrightFont = CreateFont(FONTSIZE_COPYRIGHT, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_SANS);
	DrawerAdd(StartmenuDrawer, 0, 0);
}

void StartmenuDestroy()
{
	DeleteObject(hCopyrightFont);
	DeleteObject(hDescriptionFont);
	DeleteObject(hTitleFont);
	DeleteObject(hMenuSelectedFont);
	DeleteObject(hMenuFont);
	RemoveFontResource(FONTNAME TEXT(".ttf"));
}

void StartmenuStart()
{
	static int first = 1;
	nowpos = (double)selected * itemSize.cy;
	nowvelocity = 0.0;
	if (first) {
		darking = 0;
		menuState = READY;
		first = 0;
	}
	else {
		darking = MAXDARKING;
		menuState = START;
	}
	TimerAdd(StartmenuTimer, 0, 20);
}
void StartmenuStop() {}
