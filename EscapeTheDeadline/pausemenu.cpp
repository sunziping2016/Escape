#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <math.h>
#include "pausemenu.h"

#include "timer.h"
#include "drawer.h"
#include "keyboard.h"
#include "engine.h"


#define PAUSEMENU_TITLE			TEXT("PAUSED")
#define PAUSEMENU_DESCRIPITION	TEXT("Press SPACE to select the menu.")

#define LEFT_SELECTOR			TEXT(">>>  ")
#define RIGHT_SELECTOR			TEXT("  <<<")

#define FONTNAME				TEXT("GothicE")
#define FONTNAME_SANS			TEXT("Monotype Corsiva")

#define FONTSIZE_TITLE			64
#define FONTSIZE_MENU			96
#define FONTSIZE_DESCRIPITION	48
#define FONTSIZE_SELECTOR		64
#define MARGIN					64

#define COLOR_TITLE				RGB(0xb2, 0x22, 0x22)
#define COLOR_MENUSELECTED		RGB(0xff, 0x00, 0xff)
#define COLOR_MENU				RGB(0x56, 0x3d, 0x7c)
#define COLOR_DESCRIPITION		RGB(0x00, 0x80, 0x80)
#define COLOR_BLUR				RGB(0xff, 0xf8, 0xdc)
#define COLOR_MAXR				0.9

#define STEP_COLOR				10
#define STEP_DARK				10

static TCHAR *menu[] = { TEXT("Resume"), TEXT("Main menu"), TEXT("Quit") };
#define menuEnd					(sizeof(menu) / sizeof(menu[0]))

static int selected;
static double pos[menuEnd], velocity[menuEnd], destpos[menuEnd];
static double selectorpos, selectorvelocity;
static int color[menuEnd];
static enum { FLYIN, FLYOUT, QUIT, READY } menustate;
static SIZE menuSize;
static int darkstep;

static HFONT hTitleFont, hMenuFont, hDescriptionFont, hSelectorFont;

#define EPSILON 240.0
#define PI 3.1415926535

static void PausemenuDrawer(int id, HDC hDC)
{
	static int len[menuEnd], titleLen, descripitionLen, leftselectorLen, rightselectorLen;
	static SIZE size[menuEnd], titleSize, descripitionSize, leftselectorSize, rightselectorSize;
	static int first = 1;
	int i;
	double r;
	if (!gamePaused) return;
	if (menustate != FLYOUT)
		r = 1 - fabs(pos[menuEnd - 1]) / DrawerX;
	else
		r = 1 - fabs(pos[menuEnd - 1]) / (((double)DrawerX + menuSize.cx) / 2.0 + EPSILON);
	DrawerAlphaColor(hDC, 0, 0, DrawerX, DrawerY, COLOR_BLUR, r * COLOR_MAXR);
	if (first) {
		SelectObject(hDC, hMenuFont);
		for (i = 0; i < menuEnd; ++i) {
			len[i] = (int)wcslen(menu[i]);
			GetTextExtentPoint(hDC, menu[i], len[i], &size[i]);
			if (size[i].cx > menuSize.cx) menuSize.cx = size[i].cx;
			if (size[i].cy > menuSize.cy) menuSize.cy = size[i].cy;
		}
		SelectObject(hDC, hSelectorFont);
		leftselectorLen = (int)wcslen(LEFT_SELECTOR);
		GetTextExtentPoint(hDC, LEFT_SELECTOR, leftselectorLen, &leftselectorSize);
		rightselectorLen = (int)wcslen(RIGHT_SELECTOR);
		GetTextExtentPoint(hDC, RIGHT_SELECTOR, rightselectorLen, &rightselectorSize);
		SelectObject(hDC, hTitleFont);
		titleLen = (int)wcslen(PAUSEMENU_TITLE);
		GetTextExtentPoint(hDC, PAUSEMENU_TITLE, titleLen, &titleSize);
		SelectObject(hDC, hDescriptionFont);
		descripitionLen = (int)wcslen(PAUSEMENU_DESCRIPITION);
		GetTextExtentPoint32(hDC, PAUSEMENU_DESCRIPITION, descripitionLen, &descripitionSize);
		first = 0;
	}
	for (i = 0; i < menuEnd; ++i) {
		SetTextColor(hDC, DrawerColor(COLOR_MENUSELECTED, COLOR_MENU, (double)color[i] / (STEP_COLOR - 1)));
		SelectObject(hDC, hMenuFont);
		TextOut(hDC, (DrawerX - size[i].cx) / 2 + (int)pos[i], (DrawerY - menuEnd * size[i].cy) / 2 + i * size[i].cy, menu[i], len[i]);
	}
	if (menustate == READY) {
		SetTextColor(hDC, COLOR_MENUSELECTED);
		SelectObject(hDC, hSelectorFont);
		TextOut(hDC, (DrawerX - menuSize.cx) / 2 - leftselectorSize.cx, (int)selectorpos + (DrawerY - leftselectorSize.cy) / 2, LEFT_SELECTOR, leftselectorLen);
		TextOut(hDC, (DrawerX + menuSize.cx) / 2, (int)selectorpos + (DrawerY - rightselectorSize.cy) / 2, RIGHT_SELECTOR, rightselectorLen);
		SetTextColor(hDC, COLOR_DESCRIPITION);
		SelectObject(hDC, hDescriptionFont);
		TextOut(hDC, (DrawerX - descripitionSize.cx) / 2, DrawerY - MARGIN - FONTSIZE_DESCRIPITION, PAUSEMENU_DESCRIPITION, descripitionLen);
	}
	SetTextColor(hDC, COLOR_TITLE);
	SelectObject(hDC, hTitleFont);
	TextOut(hDC, MARGIN, MARGIN, PAUSEMENU_TITLE, titleLen);
	if(menustate == QUIT)
		DrawerAlphaColor(hDC, 0, 0, DrawerX, DrawerY, RGB(0, 0, 0), sin((double)darkstep / STEP_DARK * PI / 2.0));
}

static double Factor1[menuEnd] = { 0.12, 0.08, 0.04 };
static double Factor2[menuEnd];

#define F1				0.12
#define F2				(2.0 * sqrt(F1))

static void PausemenuTimer(int id, int ms)
{
	int i;
	if (!gamePaused) return;
	if (menustate == FLYIN) {
		if (fabs(pos[menuEnd - 1] - destpos[menuEnd - 1]) < EPSILON)
			menustate = READY;
	}
	else if (menustate == FLYOUT) {
		if ((DrawerX + menuSize.cx) / 2 + pos[menuEnd - 1] < 0.0)
			EngineResume();
	}
	else if (menustate == QUIT) {
		++darkstep;
		if (darkstep == STEP_DARK)
			EngineStart(NOTSTARTED);
	}
	else if (KeyboardIsDown[VK_SPACE] || KeyboardIsDown[VK_RETURN] || KeyboardIsDown[VK_ESCAPE]) {
		if (selected == 0 || KeyboardIsDown[VK_ESCAPE]) {
			menustate = FLYOUT;
			for (i = 0; i < menuEnd; ++i)
				destpos[i] = -(DrawerX + menuSize.cx) / 2 - EPSILON;
		}
		else if (selected == 2)
			EngineStop();
		else
			menustate = QUIT;
	}
	else {
		selected = selected + KeyboardGetNum[VK_DOWN] - KeyboardGetNum[VK_UP];
		if (selected < 0)
			selected = 0;
		else if (selected >= menuEnd)
			selected = menuEnd - 1;
	}
	for (i = 0; i < menuEnd; ++i) {
		velocity[i] += Factor1[i] * (destpos[i] - pos[i]) - Factor2[i] * velocity[i];
		pos[i] += velocity[i];
		if (i == selected && color[i] < STEP_COLOR - 1)
			++color[i];
		else if (i != selected && color[i] > 1)
			--color[i];
	}
	selectorvelocity += F1 * (-(double)menuEnd * menuSize.cy / 2.0 + (selected + 0.5) * menuSize.cy - selectorpos) - F2 * selectorvelocity;
	selectorpos += selectorvelocity;
	TimerAdd(PausemenuTimer, id, ms + 20);
}

void PausemenuInit()
{
	int i;
	for (i = 0; i < menuEnd; ++i)
		Factor2[i] = 2.0 * sqrt(Factor1[i]);
	hTitleFont = CreateFont(FONTSIZE_TITLE, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_SANS);
	hMenuFont = CreateFont(FONTSIZE_MENU, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME);
	hDescriptionFont = CreateFont(FONTSIZE_DESCRIPITION, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_SANS);
	hSelectorFont = CreateFont(FONTSIZE_SELECTOR, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_SANS);
	DrawerAdd(PausemenuDrawer, 0, 0);
}
void PausemenuDestroy()
{
	DeleteObject(hSelectorFont);
	DeleteObject(hDescriptionFont);
	DeleteObject(hMenuFont);
	DeleteObject(hTitleFont);
}
void PausemenuStart()
{
	int i;
	selected = 0;
	menustate = FLYIN;
	darkstep = 0;
	for (i = 0; i < menuEnd; ++i) {
		pos[i] = DrawerX;
		velocity[i] = 0.0;
		destpos[i] = 0.0;
		color[i] = 0;
	}
	selectorpos = 0.0;
	selectorvelocity = 0.0;
	TimerAdd(PausemenuTimer, 0, 20);
}
void PausemenuStop() {}
