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
#include "commonui.h"
#include "player.h"
#include "bonus.h"

#define DOUBLEJUMP			0x01
#define LARGER				0x02
#define SMALLER				0x04
#define FLY					0x08
#define NODEATH				0x10
#define LASER				0x20
#define DEAD				0x40

#define SIZE_PLAYER			25.0

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
#define COLOR_HIGHLIFE		RGB(0x7c, 0xff, 0x00)

#define FONTSIZE_SCORE		32
#define FONTNAME_SCORE		TEXT("Monotype Corsiva")

#define VIEWPORT_DELTAX		(DrawerX / 6)

#define COOLTIME_INJURED	1500

int playerIsDied;
static HFONT hFontScore;

static COLORREF playerColorsNormal[2] = {
	RGB(0x8b, 0x00, 0x8b),			// !isOnground
	RGB(0xee, 0x00, 0xee)			// isOnground
};

static COLORREF playerColors[] = {
	RGB(0x8b, 0x00, 0x8b),			// Normal
	RGB(0xff, 0xff, 0xff),			// Nodeath
	RGB(0x7c, 0xfc, 0x00)
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
	int colorOnground;
	// Collision State
	double groundN[2];
	int isOnCollision, isOnGround;
	CollisionState collision;
	// Score and life
	int score, displayedScore;
	double life, displayedLife, lifeV;
	// State
	int state;
	// Managed by others
	double friction, jumpSpeed, jumpP[2];
	int controllable;
	// Force right?
	double forceRight;
} player;
static int hasPlayer, hasPlayerTimer;

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
	PlayerColorOff(color);
	player.colorBlink[color] = -1;
}
static void PlayerColorStopBlink(int color)
{
	PlayerColorOff(color);
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
	if (player.isOnGround && player.colorOnground < STEP_COLOR)
		++player.colorOnground;
	else if (!player.isOnGround && player.colorOnground > 0)
		--player.colorOnground;
	playerColors[0] = DrawerColor(playerColorsNormal[1], playerColorsNormal[0], (double)player.colorOnground / STEP_COLOR);
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
#define SCORE_BUFFERSIZE	100
static void PlayerScorelifeDrawer(int id, HDC hDC)
{
	HPEN hPen;
	HBRUSH hBrush;
	COLORREF fill;
	wchar_t buffer[SCORE_BUFFERSIZE];
	int len;
	SIZE size;
	if (gameState != STARTED || hasPlayer == 0) return;
	swprintf(buffer, L"Score: %6d        Distance: %8.1lf", player.displayedScore, player.pos[0]);
	len = (int)wcslen(buffer);
	SetTextColor(hDC, COLOR_SCORE);
	SelectObject(hDC, hFontScore);
	GetTextExtentPoint(hDC, buffer, len, &size);
	TextOut(hDC, DrawerX - MARGIN_PLAYER - size.cx, MARGIN_PLAYER, buffer, len);
	if (1.0 - player.displayedLife / PLAYERLIFE_MAX < 0.01) return;
	WorldSetViewport(player.pos[0] - SIZEX_LIFE / 2, player.pos[1] - player.size / 2 - DISTANCE_LIFE - SIZEY_LIFE);
	fill = DrawerColor(COLOR_HIGHLIFE, COLOR_LOWLIFE, player.displayedLife / PLAYERLIFE_MAX);
	hPen = CreatePen(PS_SOLID, 1, DrawerColor(fill, RGB(0x00, 0x00, 0x00), 0.6));
	hBrush = CreateSolidBrush(backgroundColor);
	SelectObject(hDC, hPen);
	SelectObject(hDC, hBrush);
	Rectangle(hDC, WorldX(0), WorldY(0), WorldX(SIZEX_LIFE), WorldY(SIZEY_LIFE));
	DeleteObject(hBrush);
	hBrush = CreateSolidBrush(fill);
	SelectObject(hDC, hBrush);
	Rectangle(hDC, WorldX(0), WorldY(0), WorldX(player.displayedLife / PLAYERLIFE_MAX * SIZEX_LIFE), WorldY(SIZEY_LIFE));
	DeleteObject(hBrush);
	DeleteObject(hPen);
}
#define factor1 0.1
#define factor2 (2 * sqrt(factor1))

#define DELTAN_LEN			10
#define MAX_TYPEIDNUM		20

static CollisionType types = { { ID_GROUND, ID_BONUS }, 2 };
static double orignPos[2], maxDepth;
static int onGroundTypeIds[MAX_TYPEIDNUM][2], onGroundTypeIdsEnd;
static CollisionState *PlayerCollision(int id, int othertype, int otherid)
{
	player.collision.velocity[0] = player.v[0];
	player.collision.velocity[1] = player.v[1];
	player.collision.points[0][0] = player.pos[0] - player.size / 2.0;	player.collision.points[0][1] = player.pos[1] - player.size / 2.0;
	player.collision.points[1][0] = player.pos[0] + player.size / 2.0;	player.collision.points[1][1] = player.pos[1] - player.size / 2.0;
	player.collision.points[2][0] = player.pos[0] + player.size / 2.0;	player.collision.points[2][1] = player.pos[1] + player.size / 2.0;
	player.collision.points[3][0] = player.pos[0] - player.size / 2.0;	player.collision.points[3][1] = player.pos[1] + player.size / 2.0;
	player.collision.points[4][0] = player.pos[0] - player.size / 2.0;	player.collision.points[4][1] = player.pos[1] - player.size / 2.0;
	player.collision.n = 5;
	player.collision.usev = 1;
	if (player.isOnGround)
		player.collision.usev = 0;
	/*if (othertype == ID_GROUND) {
		for (int i = 0; i < onGroundTypeIdsEnd; ++i) {
			if (onGroundTypeIds[i][0] == ID_GROUND && (otherid == onGroundTypeIds[i][1] ||
				grounds[otherid].leftId == onGroundTypeIds[i][1] && grounds[otherid].fromY == grounds[onGroundTypeIds[i][1]].toY ||
				grounds[otherid].rightId == onGroundTypeIds[i][1] && grounds[otherid].toY == grounds[onGroundTypeIds[i][1]].fromY))
				player.collision.usev = 0;
		}
	}
	player.collision.usev = 1;*/
	return &player.collision;
}
#define COLLISIONN_NUM			20
static double collisionN[COLLISIONN_NUM][3];
static int collisionNEnd;
static void PlayerOnGroundNotifier(int othertype, int otherid, double n[2], double depth, int usev)
{
	if (othertype != ID_GROUND) return;
	if (collisionNEnd != COLLISIONN_NUM) {
		collisionN[collisionNEnd][0] = n[0];
		collisionN[collisionNEnd][1] = n[1];
		collisionN[collisionNEnd][2] = depth;
		++collisionNEnd;
	}
	if (onGroundTypeIdsEnd != MAX_TYPEIDNUM) {
		onGroundTypeIds[onGroundTypeIdsEnd][0] = othertype;
		onGroundTypeIds[onGroundTypeIdsEnd][1] = otherid;
		++onGroundTypeIdsEnd;
	}
}
static void PlayerCollisionNotifier(int id, int othertype, int otherid, double n[2], double depth, int usev)
{
	double p[2] = { -n[1], n[0] };
	double newv, v[2], directionV, pos[2], lenv;
	if (othertype == ID_BONUS) {
		PlayerColorBlink(2, 1);
		return;
	}
	if (depth < maxDepth) return;
	if (usev) {
		lenv = sqrt(player.v[0] * player.v[0] + player.v[1] * player.v[1]);
		pos[0] = player.v[0] / lenv;
		pos[1] = player.v[1] / lenv;
	}
	else {
		pos[0] = n[0];
		pos[1] = n[1];
	}
	newv = p[0] * player.v[0] + p[1] * player.v[1];
	directionV = n[0] * player.v[0] + n[1] * player.v[1];
	v[0] = newv * p[0];
	v[1] = newv * p[1];
	player.isOnCollision = 1;
	// Correct the player's position
	player.v[0] = v[0];
	player.v[1] = v[1];
	player.pos[0] -= pos[0] * depth;
	player.pos[1] -= pos[1] * depth;
	player.groundN[0] = n[0];
	player.groundN[1] = n[1];
	maxDepth = depth;
}

#define VELOCITY_JUMP		20
#define ACCERATION_CONTOL	1
#define GROUND_EPSILON		1e-8

static void PlayerPosUpdate()
{
	double p[2], f[2], temp;
	int i;
	CollisionState cs;
	player.sizeV += factor1 * (player.aimedSize - player.size) - factor2 * player.sizeV;
	player.size += player.sizeV;
	cs.points[0][0] = player.collision.points[0][0] - GROUND_EPSILON; cs.points[0][1] = player.collision.points[0][1] - GROUND_EPSILON;
	cs.points[1][0] = player.collision.points[1][0] + GROUND_EPSILON; cs.points[1][1] = player.collision.points[1][1] - GROUND_EPSILON;
	cs.points[2][0] = player.collision.points[2][0] + GROUND_EPSILON; cs.points[2][1] = player.collision.points[2][1] + GROUND_EPSILON;
	cs.points[3][0] = player.collision.points[3][0] - GROUND_EPSILON; cs.points[3][1] = player.collision.points[3][1] + GROUND_EPSILON;
	cs.points[4][0] = player.collision.points[4][0] - GROUND_EPSILON; cs.points[4][1] = player.collision.points[4][1] - GROUND_EPSILON;
	cs.n = 5;
	cs.usev = 0;
	collisionNEnd = 0;
	onGroundTypeIdsEnd = 0;
	CollisionQuery(&cs, &types, PlayerOnGroundNotifier);

	/*wchar_t buffer[200] = L"", buffer2[200];
	for (int i = 0; i < onGroundTypeIdsEnd; ++i) {
		swprintf(buffer2, L"%d-%d  ", onGroundTypeIds[i][0], onGroundTypeIds[i][1]);
		wcscat(buffer, buffer2);
	}
	ErrorPrintf(L"%s", buffer);*/

	if (collisionNEnd) {
		double maxDepth = collisionN[0][3];
		player.isOnGround = 1;
		player.groundN[0] = collisionN[0][0];
		player.groundN[1] = collisionN[0][1];
		for (i = 1; i < collisionNEnd; ++i)
			if (collisionN[i][3] > maxDepth) {
				maxDepth = collisionN[i][3];
				player.groundN[0] = collisionN[i][0];
				player.groundN[1] = collisionN[i][1];
			}
	}
	else {
		player.isOnGround = 0;
		player.groundN[0] = player.groundN[1] = 0.0;
	}
	if (player.isOnGround) {
		p[0] = -player.groundN[1];  p[1] = player.groundN[0];
		player.v[0] += groundGravity * p[1] * p[0];
		player.v[1] += groundGravity * p[1] * p[1];
		temp = groundGravity * player.groundN[1] * groundFriction;
		if (sqrt(player.v[0] * player.v[0] + player.v[1] * player.v[1]) <= temp)
			player.v[0] = player.v[1] = 0.0;
		else {
			f[0] = temp * p[0];
			f[1] = temp * p[1];
			if (player.v[0] * f[0] + player.v[1] * f[1] > 0.0) {
				f[0] = -f[0];
				f[1] = -f[1];
			}
			player.v[0] += f[0];
			player.v[1] += f[1];
		}
	}
	else
		player.v[1] += groundGravity;
	if (player.isOnGround) {
		if (!commandLineFocus && KeyboardIsDown[VK_SPACE]) {
			player.v[0] -= VELOCITY_JUMP * player.groundN[0];
			player.v[1] -= VELOCITY_JUMP * player.groundN[1];
		}
		if (!commandLineFocus) {
			f[0] = ACCERATION_CONTOL * (KeyboardIsDown[VK_RIGHT] - KeyboardIsDown[VK_LEFT]);
			f[1] = ACCERATION_CONTOL * (KeyboardIsDown[VK_DOWN] - KeyboardIsDown[VK_UP]);
		}
		else
			f[0] = f[1] = 0.0;
		temp = f[0] * p[0] + f[1] * p[1];
		player.v[0] += temp * p[0];
		player.v[1] += temp * p[1];
		if (f[0] * player.groundN[0] + f[1] * player.groundN[1] < 0.0 && player.groundN[1] * groundGravity <= 0.0) {
			player.pos[0] -= 2 * GROUND_EPSILON * player.groundN[0];
			player.pos[1] -= 2 * GROUND_EPSILON * player.groundN[1];
		}
		if (player.forceRight != 0.0 && fabs(player.v[0]) < fabs(player.forceRight)) {
			if (player.forceRight > 0.0) {
				if (player.v[0] + ACCERATION_CONTOL / 2.0 < player.forceRight)
					player.v[0] += ACCERATION_CONTOL;
				else
					player.v[0] = player.forceRight;
			}
			else {
				if (player.v[0] - ACCERATION_CONTOL / 2.0  > player.forceRight)
					player.v[0] -= ACCERATION_CONTOL / 2.0;
				else
					player.v[0] = player.forceRight;
			}
		}
	}
	player.pos[0] += player.v[0];
	player.pos[1] += player.v[1];
	player.isOnCollision = 0;
	orignPos[0] = player.pos[0];
	orignPos[1] = player.pos[1];
	maxDepth = 0.0;
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
	if (gameState != STARTED || gamePaused) {
		hasPlayerTimer = 0;
		return;
	}
	PlayerColorUpdate();
	PlayerPosUpdate();
	PlayerScoreUpdate();
	PlayerLifeUpdate();
	TimerAdd(PlayerTimer, id, 20);
}
static int PlayerCreate(wchar_t *command)
{
	if (swscanf(command, L"%*s%lf%lf%lf%lf", &player.pos[0], &player.pos[1], &player.v[0], &player.v[1]) == 4) {
		hasPlayer = 1;
		return 0;
	}
	return 1;
}
static void PlayerTracked(int id, double *x, double *y)
{
	*x = player.pos[0];
	*y = player.pos[1];
}
static int nodeathTimerID;
static void PlayerNodeathTimer(int id, int ms)
{
	if (id != nodeathTimerID) return;
	player.state &= ~NODEATH;
	PlayerColorStopBlink(COLOR_NODEATH);
}
static void PlayerNodeathTimerStart(int ms)
{
	++nodeathTimerID;
	player.state |= NODEATH;
	PlayerColorStartBlink(COLOR_NODEATH);
	TimerAdd(PlayerNodeathTimer, nodeathTimerID, ms);
}
int ForceRightCreate(wchar_t *command)
{
	if (swscanf(command, L"%*s%lf", &player.forceRight) == 1)
		return 0;
	return 1;
}
void PlayerInit()
{
	hFontScore = CreateFont(FONTSIZE_SCORE, 0, 0, 0, 0, FALSE, FALSE, FALSE,
		ANSI_CHARSET, 0, 0, 0, FIXED_PITCH, FONTNAME_SCORE);
	DrawerAdd(PlayerDrawer, 0, 5);
	DrawerAdd(PlayerScorelifeDrawer, 0, 1);
	LoaderAdd(L"player", PlayerCreate);
	LoaderAdd(L"forceright", ForceRightCreate);
}
void PlayerDestroy()
{
	DeleteObject(hFontScore);
}
void PlayerStart()
{
	int i;
	playerIsDied = 0;
	player.size = 2.0;
	player.aimedSize = SIZE_PLAYER;
	player.sizeV = 0.0;
	player.groundN[0] = player.groundN[1] = 0.0;
	player.isOnCollision = player.isOnGround = 0;
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
	}
	PlayerNodeathTimerStart(COOLTIME_INJURED);
	WorldSetTracked(PlayerTracked, 0);
	CollisionAdd(PlayerCollision, 0, ID_PLAYER, &types, PlayerCollisionNotifier);
	PlayerResume();
}
void PlayerStop()
{
	CollisionRemove(PlayerCollision, 0);
	hasPlayer = 0;
	player.pos[0] = player.pos[1] = 0.0;
	player.v[0] = player.v[1] = 0.0;
	player.forceRight = 0.0;
}
void PlayerPause() {}
void PlayerResume()
{
	if (!hasPlayerTimer) {
		TimerAdd(PlayerTimer, 0, 20);
		hasPlayerTimer = 1;
	}
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
void PlayerAddLife(double num)
{
	player.life += num;
	if (player.life > PLAYERLIFE_MAX)
		player.life = PLAYERLIFE_MAX;
}
void PlayerMinusLife(double num)
{
	if (player.state & NODEATH) return;
	player.life -= num;
	if (player.life <= 0)
		player.life = 0;
	PlayerNodeathTimerStart(COOLTIME_INJURED);
}
