#include <stdio.h>
#include <math.h>
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

static struct
{
	double posX, posY;
	double vX, vY;
	double size;
	int State;
	COLORREF border, fill;
} player;
static int hasPlayer;

static void PlayerDrawer(int id, HDC hDC)
{
	POINT point;
	HPEN hPen;
	HBRUSH hBrush;
	if (gameState != STARTED || hasPlayer == 0) return;
	point = WorldSetMapper(hDC, player.posX, player.posY);
	hPen = CreatePen(PS_SOLID, 1, player.border);
	hBrush = CreateSolidBrush(player.fill);
	SelectObject(hDC, hPen);
	SelectObject(hDC, hBrush);
	Rectangle(hDC, -(int)((player.size / 2.0) + 0.5), -(int)((player.size / 2.0) + 0.5), (int)((player.size / 2.0) + 0.5), (int)((player.size / 2.0) + 0.5));
	DeleteObject(hBrush);
	DeleteObject(hPen);
	WorldResetMapper(hDC, &point);
}

int PlayerCreate(wchar_t *command)
{
	if (swscanf(command, L"%*s%lf%lf%lf%lx%lx", &player.posX, &player.posY, &player.size, &player.border, &player.fill) == 5) {
		hasPlayer = 1;
		player.border = DrawerRGB(player.border);
		player.fill = DrawerRGB(player.fill);
		return 0;
	}
	return 1;
}

void PlayerInit()
{
	DrawerAdd(PlayerDrawer, 0, 5);
	LoaderAdd(L"player", PlayerCreate);
}
void PlayerDestroy() {}
void PlayerStart()
{
	PlayerResume();
}
void PlayerStop()
{
	hasPlayer = 0;
}
void PlayerPause() {}
void PlayerResume()
{

}