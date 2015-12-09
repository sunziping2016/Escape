#pragma warning(disable: 4996)
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

#define DOUBLEJUMP			0x01
#define LARGER				0x02
#define SMALLER				0x04
#define FLY					0x08
#define NODEATH				0x10
#define LASER				0x20
#define DEAD				0x40

#define SIZE_PLAYER			20.0

#define LEFTTOP				0
#define CENTER				1
#define RIGHTBOTTOM			2
#define NUM_POINTS			3

#define COLOR_NORMAL		0
#define COLOR_NODEATH		1
#define COLOR_LANDED		2

#define MARGIN_PLAYER		32
#define DISTANCE_LIFE		16
#define SIZEX_LIFE			64
#define SIZEY_LIFE			8

#define COLOR_SCORE			RGB(0x8a, 0x2b, 0xe2)
#define COLOR_LOWLIFE		RGB(0xdc, 0x14, 0x3c)
#define COLOR_MEDIUMLIFE	RGB(0xff, 0xd7, 0x00)
#define COLOR_HIGHLIFE		RGB(0x7c, 0xff, 0x00)

#define FONTSIZE_SCORE		32
#define FONTNAME_SCORE		TEXT("Monotype Corsiva")

int playerIsDied;
static HFONT hFontScore;

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
	double size, aimedSize, sizeV;
	// Color
	int colorState[NUM_COLORS];
	int colorAlways[NUM_COLORS]; // 0 Never on, 1 Always on
	int colorBlink[NUM_COLORS];  // 0 Never Blink, -1 Always Blink, bBlink times;
	// Collision State
	double collisionN[2];
	int isOnCollision, isOnGround, isOnSqueeze[2];
	// Score and life
	int score, displayedScore;
	double life, displayedLife, lifeV;
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
#define SCORE_BUFFERSIZE	20
static void PlayerScorelifeDrawer(int id, HDC hDC)
{
	HPEN hPen;
	HBRUSH hBrush;
	COLORREF fill;
	wchar_t buffer[SCORE_BUFFERSIZE];
	int len;
	SIZE size;
	if (gameState != STARTED || hasPlayer == 0) return;
	swprintf(buffer, L"Score: %06d", player.displayedScore);
	len = (int)wcslen(buffer);
	SetTextColor(hDC, COLOR_SCORE);
	SelectObject(hDC, hFontScore);
	GetTextExtentPoint(hDC, buffer, len, &size);
	TextOut(hDC, DrawerX - MARGIN_PLAYER - size.cx, MARGIN_PLAYER, buffer, len);
	if (1.0 - player.displayedLife / PLAYERLIFE_MAX < 0.01) return;
	WorldSetViewport(player.pos[0] - SIZEX_LIFE / 2, player.pos[1] - player.size / 2 - DISTANCE_LIFE - SIZEY_LIFE);
	fill = DrawerColor(COLOR_HIGHLIFE, COLOR_LOWLIFE, player.displayedLife / PLAYERLIFE_MAX);
	hPen = CreatePen(PS_SOLID, 1, DrawerColor(fill, RGB(0x00, 0x00, 0x00), 0.6));
	hBrush = CreateSolidBrush(groundColor);
	SelectObject(hDC, hPen);
	SelectObject(hDC, hBrush);
	Rectangle(hDC, WorldX(0), WorldY(0), WorldX(SIZEX_LIFE), WorldY(SIZEY_LIFE));
	DeleteObject(hBrush);
	hBrush = CreateSolidBrush(fill);
	SelectObject(hDC, hBrush);
	Rectangle(hDC, WorldX(0), WorldY(0), WorldX(player.displayedLife / PLAYERLIFE_MAX * SIZEX_LIFE), WorldY(SIZEY_LIFE));
	DeleteObject(hBrush);
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
	player.sizeV += factor1 * (player.aimedSize - player.size) - factor2 * player.sizeV;
	player.size += player.sizeV;
	if (player.isOnGround) {
		if (KeyboardIsDown[VK_SPACE]) {
			player.v[0] -= VELOCITY_JUMP * player.collisionN[0];
			player.v[1] -= VELOCITY_JUMP * player.collisionN[1];
		}
		player.v[0] += ACCERATION_CONTOL * (KeyboardIsDown[VK_RIGHT] - KeyboardIsDown[VK_LEFT]);
		//player.v[1] += ACCERATION_CONTOL * (KeyboardIsDown[VK_DOWN] - KeyboardIsDown[VK_UP]);
	}
	if (KeyboardIsDown['Q'])
		PlayerMinusLife(2);
	if (KeyboardIsDown['P'])
		PlayerAddLife(100);
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
static void PlayerScoreUpdate()
{
	if (player.displayedScore < player.score)
		++player.displayedScore;
	else if (player.displayedScore > player.score)
		--player.displayedScore;
}
static void PlayerLifeUpdate()
{
	player.lifeV += factor1 * (player.life - player.displayedLife) - factor2 * player.lifeV;
	player.displayedLife += player.lifeV;
	if(player.displayedLife < 0.01 * PLAYERLIFE_MAX)
		PlayerDie();
}
static void PlayerTimer(int id, int ms)
{
	if (gameState != STARTED || gamePaused) return;
	PlayerColorUpdate();
	PlayerPosUpdate();
	PlayerScoreUpdate();
	PlayerLifeUpdate();
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
	hFontScore = CreateFont(FONTSIZE_SCORE, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_SCORE);
	DrawerAdd(PlayerDrawer, 0, 5);
	DrawerAdd(PlayerScorelifeDrawer, 0, 1);
	LoaderAdd(L"player", PlayerCreate);
}
void PlayerDestroy()
{
	DeleteObject(hFontScore);
}
void PlayerStart()
{
	int i;
	playerIsDied = 0;
	player.v[0] = player.v[1] = 0.0;
	player.size = 2.0;
	player.aimedSize = SIZE_PLAYER;
	player.sizeV = 0.0;
	player.collisionN[0] = player.collisionN[1] = 0.0;
	player.isOnCollision = player.isOnGround = player.isOnSqueeze[0] = player.isOnSqueeze[0] = 0;
	player.score = player.displayedScore = 0;
	player.life = player.displayedLife = PLAYERLIFE_MAX;
	player.lifeV = 0.0;
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
void PlayerAddScore(int num)
{
	player.score += num;
}
int PlayerGetScore()
{
	return player.score;
}
void PlayerDie()
{
	playerIsDied = 1;
	EngineStart(DIED);
}
void PlayerAddLife(int num)
{
	player.life += num;
	if (player.life > PLAYERLIFE_MAX)
		player.life = PLAYERLIFE_MAX;
}
void PlayerMinusLife(int num)
{
	player.life -= num;
	if (player.life <= 0)
		player.life = 0;
}
