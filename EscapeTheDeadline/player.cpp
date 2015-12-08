#include <stdio.h>
#include <math.h>
#include <string.h>
#include "keyboard.h"
#include "drawer.h"
#include "timer.h"
#include "world.h"
#include "engine.h"
#include "loader.h"
#include "collision.h"
#include "ground.h"

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
#define COLOR_LANDED	2

static COLORREF playerColors[] = {
	RGB(0x56, 0x3d, 0x7c),			// Normal
	RGB(0xff, 0xff, 0xff),			// Nodeath
	RGB(0x8b, 0x00, 0x8b)			// Landed
};

#define NUM_COLORS		(sizeof(playerColors) / sizeof(playerColors[0]))

static struct
{
	// Position and Velocity
	double pos[2], v[2];
	// Body size
	double size, sizev;
	// Color
	int colorState[NUM_COLORS];
	int colorAlways[NUM_COLORS]; // 0 Never on, 1 Always on
	int colorBlink[NUM_COLORS];  // 0 Never Blink, -1 Always Blink, bBlink times;
	// Collision State
	double collisionN[2];
	int isOnCollision, isOnGround, isOnSqueeze[2];
	// Score
	int score;
	// State
	int state;
} player;
static int hasPlayer;

#define STEP_COLOR	10
#define GetColorStep(colorstate)	(STEP_COLOR - abs(player.colorState[i] % (2 * STEP_COLOR) - STEP_COLOR))

static void PlayerColorOn(int color)
{
	player.colorAlways[COLOR_NORMAL] = 0;
	player.colorAlways[color] = 1;
}
static void PlayerColorOff(int color)
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
	player.colorBlink[color] = time;
}
static void PlayerColorStartBlink(int color)
{
	player.colorAlways[color] = 0;
	player.colorBlink[color] = -1;
}
static void PlayerColorStopBlink(int color)
{
	player.colorAlways[color] = 0;
	player.colorBlink[color] = 0;
}
static void PlayerColorUpdate()
{
	int i, state;
	for (i = 0; i < NUM_COLORS; ++i) {
		if (player.colorAlways[i] == 1){
			state = GetColorStep(player.colorState[i]);
			if (state != STEP_COLOR)
				player.colorState[i] = state + 1;
			else if (player.colorBlink[i] == -1)
				PlayerColorOff(i);
			else if (player.colorBlink[i] > 0) {
				player.colorBlink[i] -= 1;
				PlayerColorOff(i);
			}
		}
		else {
			if (player.colorState[i]>0)
				--player.colorState[i];
			else if (player.colorBlink[i] != 0)
				PlayerColorOn(i);
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
	HPEN hPen;
	HBRUSH hBrush;
	COLORREF fill;
	if (gameState != STARTED || hasPlayer == 0) return;
	WorldSetViewport(player.pos[0], player.pos[1]);
	fill = PlayerColorFill();
	hPen = CreatePen(PS_SOLID, 4, PlayerColorBorder(fill));
	hBrush = CreateSolidBrush(fill);
	SelectObject(hDC, hPen);
	SelectObject(hDC, hBrush);
	Rectangle(hDC, WorldX(-player.size / 2.0), WorldY(-player.size / 2.0), WorldX(player.size / 2.0), WorldY(player.size / 2.0));
	DeleteObject(hBrush);
	DeleteObject(hPen);
}
#define factor1 0.1
#define factor2 (2 * sqrt(factor1))

#define DELTAN_LEN		10

static Types types = { { ID_BORDER }, 1 };
static double orignPos[2], maxDelta[2];
static double deltaN[DELTAN_LEN][3];
static int deltaNEnd;
static Points *PlayerCollision(int id)
{
	static Points points;
	points.points[0][0] = player.pos[0] - player.size / 2.0;		points.points[0][1] = player.pos[1] - player.size / 2.0;
	points.points[1][0] = player.pos[0] + player.size / 2.0;		points.points[1][1] = player.pos[1] - player.size / 2.0;
	points.points[2][0] = player.pos[0] + player.size / 2.0;		points.points[2][1] = player.pos[1] + player.size / 2.0;
	points.points[3][0] = player.pos[0] - player.size / 2.0;		points.points[3][1] = player.pos[1] + player.size / 2.0;
	points.points[4][0] = player.pos[0] - player.size / 2.0;		points.points[4][1] = player.pos[1] - player.size / 2.0;
	points.n = 5;
	return &points;
}
static void PlayerCollisionNotifier(int id, int othertype, int otherid, double n[2], double depth)
{
	int i, j;
	double delta, p[2] = { -n[1], n[0] };
	double newv, v[2], directionV, sum;
	newv = p[0] * player.v[0] + p[1] * player.v[1];
	directionV = n[0] * player.v[0] + n[1] * player.v[1];
	v[0] = newv * p[0];
	v[1] = newv * p[1];
	player.isOnCollision = 1;
	if (othertype == ID_BORDER)
		player.isOnGround = 1;
	if (deltaNEnd != DELTAN_LEN) {
		deltaN[deltaNEnd][0] = n[0];
		deltaN[deltaNEnd][1] = n[1];
		deltaN[deltaNEnd][3] = depth;
		++deltaNEnd;
	}
	// Correct the player's position
	for (i = 0; i < 2; ++i) {
		if (!player.isOnSqueeze[i]) {
			delta = depth * n[i];
			if (maxDelta[i] * delta >= 0.0) {
				if (fabs(delta) >= fabs(maxDelta[i])) {
					maxDelta[i] = delta;
					player.pos[i] = orignPos[i] - delta;
					if (directionV > 0.0)
						player.v[i] = v[i];
					sum = 0.0;
					for (j = 0; j < deltaNEnd; ++j)
						sum += deltaN[j][i];
					player.collisionN[i] = sum / deltaNEnd;
				}
			}
			else
				player.isOnSqueeze[i] = 1;
		}
	}
}

#define VELOCITY_JUMP		20
#define ACCERATION_CONTOL	1

static void PlayerPosUpdate()
{
	double p[2], f[2];
	player.sizev += factor1 * (SIZE_PLAYER - player.size) - factor2 * player.sizev;
	player.size += player.sizev;
	if (player.isOnGround) {
		if (KeyboardIsDown[VK_SPACE]) {
			player.v[0] -= VELOCITY_JUMP * player.collisionN[0];
			player.v[1] -= VELOCITY_JUMP * player.collisionN[1];
		}
		player.v[0] += ACCERATION_CONTOL * (KeyboardIsDown[VK_RIGHT] - KeyboardIsDown[VK_LEFT]);
		//player.v[1] += ACCERATION_CONTOL * (KeyboardIsDown[VK_DOWN] - KeyboardIsDown[VK_UP]);
	}
	if (player.isOnGround) {
		p[0] = -player.collisionN[1];  p[1] = player.collisionN[0];
		player.v[0] += groundGravity * p[1] * p[0];
		player.v[1] += groundGravity * p[1] * p[1];
		if (sqrt(player.v[0] * player.v[0] + player.v[1] * player.v[1]) < groundGravity * groundFriction)
			player.v[0] = player.v[1] = 0.0;
		else {
			f[0] = groundGravity * groundFriction * p[0];
			f[1] = groundGravity * groundFriction * p[1];
			if (player.v[0] * f[0] + player.v[1] * f[1] > 0.0) {
				f[0] = -f[0];
				f[1] = -f[1];
			}
			player.v[0] += f[0];
			player.v[1] -= f[1];
		}
	}
	else
		player.v[1] += groundGravity;
	player.pos[0] += player.v[0];
	player.pos[1] += player.v[1];
	player.collisionN[0] = player.collisionN[1] = 0.0;
	player.isOnCollision = player.isOnGround = player.isOnSqueeze[0] = player.isOnSqueeze[0] = 0;
	orignPos[0] = player.pos[0];
	orignPos[1] = player.pos[1];
	maxDelta[0] = maxDelta[1] = 0.0;
	deltaNEnd = 0;
}

static void PlayerTimer(int id, int ms)
{
	if (gameState != STARTED || gamePaused) return;
	PlayerColorUpdate();
	PlayerPosUpdate();
	TimerAdd(PlayerTimer, id, 20);
}
static int PlayerCreate(wchar_t *command)
{
	if (swscanf(command, L"%*s%lf%lf", &player.pos[0], &player.pos[1]) == 2) {
		hasPlayer = 1;
		return 0;
	}
	return 1;
}
static void PlayerTracked(int id, double *x, double *y)
{
	*x = player.pos[0];	*y = player.pos[1];
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
	player.v[0] = player.v[1] = 0.0;
	player.sizev = 0.0;
	player.size = 2.0;
	player.collisionN[0] = player.collisionN[1] = 0.0;
	player.isOnCollision = player.isOnGround = player.isOnSqueeze[0] = player.isOnSqueeze[0] = 0;
	player.state = 0;
	for (i = 0; i < NUM_COLORS; ++i) {
		if (i == COLOR_NORMAL) {
			player.colorState[i] = STEP_COLOR;
			player.colorAlways[i] = 1;
			player.colorBlink[i] = 0;
		}
		else {
			player.colorState[i] = 0;
			player.colorAlways[i] = 0;
			player.colorBlink[i] = 0;
		}
	};
	PlayerColorBlink(COLOR_NODEATH, 3);
	WorldSetTracked(PlayerTracked, 0);
	CollisionAdd(PlayerCollision, 0, ID_PLAYER, &types, PlayerCollisionNotifier);
	PlayerResume();
}
void PlayerStop()
{
	CollisionRemove(PlayerCollision, 0);
	hasPlayer = 0;
	player.pos[0] = player.pos[1] = 0.0;
}
void PlayerPause() {}
void PlayerResume()
{
	TimerAdd(PlayerTimer, 0, 20);
}
