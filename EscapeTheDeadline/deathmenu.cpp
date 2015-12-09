#pragma warning(disable: 4996)
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "deathmenu.h"
#include "drawer.h"
#include "timer.h"
#include "engine.h"
#include "keyboard.h"
#include "player.h"

#define DEATHMENU_LOSETITLE		TEXT("You lose!")
#define DEATHMENU_WINTITLE		TEXT("You win!")
#define DEATHMENU_DESCRIPITION	TEXT("Press SPACE to go back to the main menu.")

#define MARGIN_DEATHMENU		64

#define COLOR_TITLE_LOSE		RGB(0x8b, 0x00, 0x00)
#define COLOR_TITLE_WIN			RGB(0x84, 0x70, 0xff)
#define COLOR_SCORE				RGB(0x56, 0x3d, 0x7c)
#define COLOR_DESCRIPITION		RGB(0x00, 0x80, 0x80)
#define COLOR_BK				RGB(0xff, 0xf8, 0xdc)


#define FONTSIZE_TITLE			108
#define FONTSIZE_SCORE			96
#define FONTSIZE_DESCRIPITION	48

#define FONTNAME_TITLE			TEXT("GothicE")
#define FONTNAME_SCORE			TEXT("Monotype Corsiva")
#define FONTNAME_DESCRIPITION	TEXT("Monotype Corsiva")

#define STEP_DARK				10

#define DEATHMENU_BUFFERSIZE	30

static TCHAR *deathmenuTitle;
static TCHAR deathmenuScore[DEATHMENU_BUFFERSIZE];
static COLORREF deathmenuTitleColor;
static enum { START, READY, END } menuState;
static HFONT hTitleFont, hScoreFont, hDescripitionFont;
static HBRUSH hBrushBackground;
static int darkstep;

#define PI 3.1415926535

void DeathmenuDrawer(int id, HDC hDC)
{
	static int titleLen, scoreLen, descripitionLen;
	static SIZE titleSize, scoreSize, descripitionSize;
	static int first = 1;
	RECT rect;
	if (gameState != DIED) return;
	if (first) {
		SelectObject(hDC, hTitleFont);
		titleLen = (int)wcslen(deathmenuTitle);
		GetTextExtentPoint(hDC, deathmenuTitle, titleLen, &titleSize);
		SelectObject(hDC, hScoreFont);
		scoreLen = (int)wcslen(deathmenuScore);
		GetTextExtentPoint(hDC, deathmenuScore, scoreLen, &scoreSize);
		SelectObject(hDC, hDescripitionFont);
		descripitionLen = (int)wcslen(DEATHMENU_DESCRIPITION);
		GetTextExtentPoint(hDC, DEATHMENU_DESCRIPITION, descripitionLen, &descripitionSize);
	}
	rect.left = 0;			rect.top = 0;
	rect.right = DrawerX;	rect.bottom = DrawerY;
	FillRect(hDC, &rect, hBrushBackground);
	SelectObject(hDC, hTitleFont);
	SetTextColor(hDC, deathmenuTitleColor);
	TextOut(hDC, (DrawerX - titleSize.cx) / 2, MARGIN_DEATHMENU, deathmenuTitle, titleLen);
	SelectObject(hDC, hScoreFont);
	SetTextColor(hDC, COLOR_SCORE);
	TextOut(hDC, (DrawerX - scoreSize.cx) / 2, (DrawerY - scoreSize.cy) / 2, deathmenuScore, scoreLen);
	SelectObject(hDC, hDescripitionFont);
	SetTextColor(hDC, COLOR_DESCRIPITION);
	TextOut(hDC, (DrawerX - descripitionSize.cx) / 2, DrawerY - descripitionSize.cy - MARGIN_DEATHMENU, DEATHMENU_DESCRIPITION, descripitionLen);
	if (menuState == START || menuState == END)
		DrawerAlphaColor(hDC, 0, 0, DrawerX, DrawerY, RGB(0, 0, 0), sin((double)darkstep / STEP_DARK * PI / 2.0));
}
void DeathmenuTimer(int id, int ms)
{
	if (gameState != DIED) return;
	if (menuState == START) {
		--darkstep;
		if (darkstep == 0)
			menuState = READY;
	}
	else if (menuState == END) {
		++darkstep;
		if (darkstep == STEP_DARK)
			EngineStart(NOTSTARTED);
	}
	else if (KeyboardIsDown[VK_SPACE] || KeyboardIsDown[VK_RETURN])
		menuState = END;
	else if (KeyboardIsDown[VK_ESCAPE])
		EngineStop();
	TimerAdd(DeathmenuTimer, 0, ms + 20);
}
void DeathmenuInit()
{
	hTitleFont = CreateFont(FONTSIZE_TITLE, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_TITLE);
	hScoreFont = CreateFont(FONTSIZE_SCORE, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_SCORE);
	hDescripitionFont = CreateFont(FONTSIZE_DESCRIPITION, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_DESCRIPITION);
	hBrushBackground = CreateSolidBrush(COLOR_BK);
	DrawerAdd(DeathmenuDrawer, 0, 0);
}
void DeathmenuDestroy()
{
	DeleteObject(hBrushBackground);
	DeleteObject(hDescripitionFont);
	DeleteObject(hScoreFont);
	DeleteObject(hTitleFont);
}
void DeathmenuStart()
{
	menuState = START;
	darkstep = STEP_DARK;
	if (playerIsDied) {
		deathmenuTitle = DEATHMENU_LOSETITLE;
		deathmenuTitleColor = COLOR_TITLE_LOSE;
	}
	else {
		deathmenuTitle = DEATHMENU_WINTITLE;
		deathmenuTitleColor = COLOR_TITLE_WIN;
	}
	swprintf(deathmenuScore, L"Your score: %06d", PlayerGetScore());
	TimerAdd(DeathmenuTimer, 0, 20);
}
void DeathmenuStop() {}
