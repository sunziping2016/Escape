#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <math.h>
#include "startmenu.h"

#include "timer.h"
#include "drawer.h"
#include "keyboard.h"
#include "engine.h"

#define GAMEFILE_MAXLEN			50

#define GAME_TITLE				TEXT("Escape the Deadline")
#define GAME_PROMPT				TEXT("Choose level: ")
#define GAME_DESRIPITION		TEXT("Press SPACE to start the game.")

#define FONTNAME				TEXT("GothicE")
#define FONTNAME_SANS			TEXT("Monotype Corsiva")

#define FONTSIZE_ITEM			64
#define FONTSIZE_TITLE			96
#define FONTSIZE_DESRIPITION	48
#define MARGIN					64

#define COLOR_TITLE				RGB(0xb2, 0x22, 0x22)
#define COLOR_ITEM				RGB(0x56, 0x3d, 0x7c)
#define COLOR_DESRIPITION		RGB(0x00, 0x80, 0x80)
#define COLOR_PROMPT			RGB(0x80, 0x80, 0x00)
#define COLOR_BK				RGB(0xff, 0xf8, 0xdc)
#define COLOR_BLACK				RGB(0x40, 0x40, 0x40)

#define MAXDARKING				15

#define GAMEFILE_DIR			TEXT("games//")
#define GAMEFILE_EXTENSION		TEXT(".txt")

#define MIXCOLOR(a, b, r)		RGB((int)(r * GetRValue(a) + (1 - r) * GetRValue(b)), \
									(int)(r * GetGValue(a) + (1 - r) * GetGValue(b)), \
									(int)(r * GetBValue(a) + (1 - r) * GetBValue(b)))

static TCHAR gamefiles[GAMEFILE_MAXLEN][MAX_PATH];
static int gamefilesEnd = 0;

static HFONT hMenuFont, hMenuSelectedFont, hTitleFont, hDesriptionFont;
static SIZE itemSize;
static int selected;
static double nowpos, nowvelocity;
static int darking;

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

static void StartmenuDrawer(int id, HDC hDC)
{
	HBRUSH hBkColor;
	static int len[GAMEFILE_MAXLEN], titleLen, descripitionLen, promptLen;
	static SIZE size[GAMEFILE_MAXLEN], titleSize, descripitionSize, promptSize;
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
		SelectObject(hDC, hDesriptionFont);
		descripitionLen = (int)wcslen(GAME_DESRIPITION);
		GetTextExtentPoint32(hDC, GAME_DESRIPITION, descripitionLen, &descripitionSize);
		first = 0;
	}
	r1 = (double)darking / MAXDARKING;
	hBkColor = CreateSolidBrush(MIXCOLOR(COLOR_BLACK, COLOR_BK, r1));
	FillRect(hDC, &rect, hBkColor);
	DeleteObject(hBkColor);
	SetBkMode(hDC, TRANSPARENT);
	num = (double)(DrawerY - 2 * MARGIN - FONTSIZE_DESRIPITION - FONTSIZE_TITLE) / FONTSIZE_ITEM / 2.0 - 1.0;
	for (i = 0; i < gamefilesEnd; ++i) {
		distance = fabs((double)i * itemSize.cy - nowpos + 0.5);
		if (distance >= num * itemSize.cy) continue;
		r2 = distance / (num * itemSize.cy);
		SetTextColor(hDC, MIXCOLOR(COLOR_BLACK, MIXCOLOR(COLOR_BK, COLOR_ITEM, r2), r1));
		if (i == selected) SelectObject(hDC, hMenuSelectedFont);
		else SelectObject(hDC, hMenuFont);
		TextOut(hDC, (DrawerX - itemSize.cx + promptSize.cx) / 2, (DrawerY - itemSize.cy) / 2 - (int)(nowpos + 0.5) + itemSize.cy * i, gamefiles[i], len[i]);
	}
	SetTextColor(hDC, MIXCOLOR(COLOR_BLACK, COLOR_PROMPT, r1));
	SelectObject(hDC, hMenuFont);
	TextOut(hDC, (DrawerX - itemSize.cx - promptSize.cx) / 2, (DrawerY - promptSize.cy) / 2, GAME_PROMPT, promptLen);
	SetTextColor(hDC, MIXCOLOR(COLOR_BLACK, COLOR_TITLE, r1));
	SelectObject(hDC, hTitleFont);
	TextOut(hDC, (DrawerX - titleSize.cx) / 2, MARGIN, GAME_TITLE, titleLen);
	SetTextColor(hDC, MIXCOLOR(COLOR_BLACK, COLOR_DESRIPITION, r1));
	SelectObject(hDC, hDesriptionFont);
	TextOut(hDC, (DrawerX - descripitionSize.cx) / 2, DrawerY - MARGIN - FONTSIZE_DESRIPITION, GAME_DESRIPITION, descripitionLen);
}

#define factor1 0.05
#define factor2 (2 * sqrt(factor1))

static void StartmenuTimer(int id, int ms)
{
	double distance;
	if (gameState != NOTSTARTED || gamefilesEnd == 0) return;
	if (KeyboardIsDown[VK_ESCAPE]) EngineStop();
	if (darking) {
		if (darking > MAXDARKING) EngineStart(STARTED);
		++darking;
	}
	else if (KeyboardIsDown[VK_SPACE] || KeyboardIsDown[VK_RETURN])
		++darking;
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
	hDesriptionFont = CreateFont(FONTSIZE_DESRIPITION, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_SANS);
	DrawerAdd(StartmenuDrawer, 0, 0);
}

void StartmenuDestroy()
{
	DeleteObject(hTitleFont);
	DeleteObject(hMenuSelectedFont);
	DeleteObject(hMenuFont);
	RemoveFontResource(FONTNAME TEXT(".ttf"));
}

void StartmenuStart()
{
	selected = 0;
	nowpos = nowvelocity = 0.0;
	darking = 0;
	TimerAdd(StartmenuTimer, 0, 20);
}
void StartmenuStop() {}
