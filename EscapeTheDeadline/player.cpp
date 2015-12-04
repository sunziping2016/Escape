#include <stdio.h>
#include <math.h>
#include <string.h>
#include "drawer.h"
#include "timer.h"
#include "world.h"
#include "engine.h"
#include "loader.h"

#include "player.h"

#define DOUBLEJUMP		0x01
#define LARGER			0x02
#define SMALLER			0x04
#define FLY				0x08
#define NODEATH			0x10
#define LASER			0x20
#define DEAD			0x40

#define SIZE_PLAYER		20.0

#define LEFTTOP			0
#define CENTER			1
#define RIGHTBOTTOM		2
#define NUM_POINTS		3

#define COLOR_NORMAL	0
#define COLOR_NODEATH	1

static COLORREF playerColors[] = {
	RGB(0x56, 0x3d, 0x7c),			// Normal
	RGB(0xff, 0xff, 0x00),			// Nodeath
};

#define NUM_COLORS		(sizeof(playerColors) / sizeof(playerColors[0]))

static struct
{
	// LEFTTOP and RIGHTBOTTOM use relative coordinate.
	double posX[3], posY[3];
	double vX[3], vY[3];
	int state;
	int colorState[NUM_COLORS];
	int colorAlways[NUM_COLORS]; // 0 Normal,1 Always,2 Blink
} player;
static int hasPlayer;

#define STEP_COLOR	15
#define GetColorStep(colorstate)	(STEP_COLOR - abs(player.colorState[i] % (2 * STEP_COLOR) - STEP_COLOR))

static void PlayerColorSet(int color)
{
	player.colorAlways[COLOR_NORMAL] = 0;
	player.colorAlways[color] = 1;
}
static void PlayerColorReset(int color)
{
	int i;
	player.colorAlways[color] = 0;
	for (i = 0; i < NUM_COLORS; ++i)
		if (player.colorAlways[color] == 1)
			return;
	player.colorAlways[COLOR_NORMAL] = 1;
}
static void PlayerColorBlink(int color, int time)
{
	player.colorAlways[color] = 0;
	player.colorState[color] += time * (2 * STEP_COLOR);
}
static void PlayerColorStartBlink(int color)
{
	player.colorAlways[color] = 2;
}
static void PlayerColorStopBlink(int color)
{
	player.colorAlways[color] = 0;
}
static void PlayerColorUpdate()
{
	int i, state;
	for (i = 0; i < NUM_COLORS; ++i) {
		if (player.colorAlways[i] == 2) {
			player.colorState[i] = (player.colorState[i] + 1) % (2 * STEP_COLOR);
		}
		else if (player.colorAlways[i] == 1){
			state = GetColorStep(player.colorState[i]);
			if (state != STEP_COLOR)
				player.colorState[i] = state + 1;
		}
		else {
			if (player.colorState[i]>0)
				--player.colorState[i];
		}
	}

}
static COLORREF PlayerColorFill()
{
	int i, sum = 0;
	double r = 0.0, g = 0.0, b = 0.0;
	for (i = 0; i < NUM_COLORS; ++i)
		sum += GetColorStep(player.colorState[i]);
	for (i = 0; i < NUM_COLORS; ++i) {
		r += (double)GetColorStep(player.colorState[i]) / sum * GetRValue(playerColors[i]);
		g += (double)GetColorStep(player.colorState[i]) / sum * GetGValue(playerColors[i]);
		b += (double)GetColorStep(player.colorState[i]) / sum * GetBValue(playerColors[i]);
	}
	return RGB((int)(r + 0.5), (int)(g + 0.5), (int)(b + 0.5));
}
static COLORREF PlayerColorBorder(COLORREF fill)
{
	return DrawerColor(fill, RGB(0, 0, 0), 0.8);
}

static void PlayerDrawer(int id, HDC hDC)
{
	POINT point;
	HPEN hPen;
	HBRUSH hBrush;
	COLORREF fill;
	if (gameState != STARTED || hasPlayer == 0) return;
	point = WorldSetMapper(hDC, player.posX[CENTER], player.posY[CENTER]);
	fill = PlayerColorFill();
	hPen = CreatePen(PS_SOLID, 1, PlayerColorBorder(fill));
	hBrush = CreateSolidBrush(fill);
	SelectObject(hDC, hPen);
	SelectObject(hDC, hBrush);
	Rectangle(hDC, (int)(player.posX[LEFTTOP] - 0.5), (int)(player.posY[LEFTTOP] - 0.5), (int)(player.posX[RIGHTBOTTOM] + 0.5), (int)(player.posY[RIGHTBOTTOM] + 0.5));
	DeleteObject(hBrush);
	DeleteObject(hPen);
	WorldResetMapper(hDC, &point);
}
static double PlayerGetAimedPos(int pos)
{
	if (pos == LEFTTOP)
		return -SIZE_PLAYER / 2.0;
	else if (pos = RIGHTBOTTOM)
		return SIZE_PLAYER / 2.0;
	else
		return 0.0;
}

#define factor1 0.5
#define factor2 (2 * sqrt(factor1))

static void PlayerPosUpdate()
{
	int i;
	double a;
	for (i = 0; i < NUM_POINTS; ++i) {
		if (i == CENTER) continue;
		a = factor1 * (PlayerGetAimedPos(i) - player.posX[i]) - factor2 * player.vX[i];
		player.vX[i] += a; player.vX[CENTER] -= a;
		player.posX[i] += player.vX[i];
		a = factor1 * (PlayerGetAimedPos(i) - player.posY[i]) - factor2 * player.vY[i];
		player.vY[i] += a; player.vY[CENTER] -= a;
		player.posY[i] += player.vY[i];
	}
}

static void PlayerTimer(int id, int ms)
{
	if (gameState != STARTED || gamePaused) return;
	PlayerColorUpdate();
	PlayerPosUpdate();
	TimerAdd(PlayerTimer, id, 20);
}

int PlayerCreate(wchar_t *command)
{
	if (swscanf(command, L"%*s%lf%lf", &player.posX[CENTER], &player.posY[CENTER]) == 2) {
		hasPlayer = 1;
		return 0;
	}
	return 1;
}
void PlayerTracked(int id, double *x, double *y)
{
	*x = player.posX[CENTER];
	*y = player.posY[CENTER];
}

void PlayerInit()
{
	DrawerAdd(PlayerDrawer, 0, 5);
	LoaderAdd(L"player", PlayerCreate);
}
void PlayerDestroy() {}
void PlayerStart()
{
	int i;
	player.posX[LEFTTOP] = player.posY[LEFTTOP] = -2.0;
	player.posX[RIGHTBOTTOM] = player.posY[RIGHTBOTTOM] = 2.0;
	memset(player.vX, 0, sizeof(player.vX));
	memset(player.vY, 0, sizeof(player.vY));
	player.state = 0;
	for (i = 0; i < NUM_COLORS; ++i) {
		if (i == COLOR_NORMAL) {
			player.colorState[i] = STEP_COLOR;
			player.colorAlways[i] = 1;
		}
		else {
			player.colorState[i] = 0;
			player.colorAlways[i] = 0;
		}
	};
	PlayerColorStartBlink(COLOR_NODEATH);
	WorldSetTracked(PlayerTracked, 0);
	PlayerResume();
}
void PlayerStop()
{
	hasPlayer = 0;
	player.posX[CENTER] = player.posY[CENTER] = 0.0;
}
void PlayerPause() {}
void PlayerResume()
{
	TimerAdd(PlayerTimer, 0, 20);
}