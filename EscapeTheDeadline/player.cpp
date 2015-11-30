#include <math.h>
#include "drawer.h"
#include "timer.h"
#include "world.h"
#include "engine.h"

#include "player.h"

static struct
{
	double rHead, rStick;
	double lHead, lBody, lUpperarm, lForearm, lThigh, lShank, lFoot;
	COLORREF color;
} StickmanPara = { 15, 5, 16, 45, 26, 26, 35, 35, 7, RGB(0, 0, 0)};

static struct
{
	double aBody, aHead;
	double aUpperarm1, aForearm1;
	double aUpperarm2, aForearm2;
	double aThigh1, aShank1, aFoot1;
	double aThigh2, aShank2, aFoot2;
} StickManState[] = {
	{ 84, 78,  -60,   46, -150,  -95,  -20, -144,  -45, -95, -130, -40 }, // Run to right BEGIN
	{ 84, 78,  -86,   15, -130, -105,  -33, -170,  -98, -86, -110,  -8 },
	{ 84, 78,  -90,  -22, -112,  -84,  -58,  170, -142, -78,  -98,   0 },
	{ 82, 74,  -90,  -66, -104,  -36,  -76,  156, -178, -52,  -72,   5 },
	{ 80, 74, -106,  -78,  -96,  -10,  -86,  160, -178, -44,  -70,   8 },
	{ 82, 76, -132,  -70,  -78,   15, -100,  170, -134, -42,  -68,   0 },
	{ 82, 78, -152,  -86,  -40,   90, -110, -172, -106, -16,  -80,  -4 },
	{ 84, 78, -160,  -98,  -44,   78, -124, -145,  -75, -14, -100, -30 },
	{ 84, 78, -160, -100,  -56,   65, -112, -140,  -75, -15, -122, -38 }  // Run to right END
};

#define PI 3.1415926535

void getPoint(double x, double y, double length, double angle, double *dx, double *dy)
{
	*dx = x + length * cos(angle * PI / 180);
	*dy = y - length * sin(angle * PI / 180);
}

void DrawStick(HDC hdc, double x1, double y1, double x2, double y2, double r)
{
	double distance = sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
	double dx = (y1 - y2) / distance * r, dy = (x2 - x1) / distance * r;
	POINT points[4] = { { round(x1 - dx), round(y1 - dy) },{ round(x2 - dx), round(y2 - dy) },
		{ round(x2 + dx), round(y2 + dy) },{ round(x1 + dx), round(y1 + dy) } };
	Ellipse(hdc, round(x1 - r), round(y1 - r), round(x1 + r), round(y1 + r));
	Ellipse(hdc, round(x2 - r), round(y2 - r), round(x2 + r), round(y2 + r));
	Polygon(hdc, points, 4);
}

static struct {
	double x;
	double y;
	int state;
} Player[1];

void StickmanDraw(int id, HDC hdc)
{
	double ux, uy, nx, ny, dx, dy;
	double x = 0, y = 0;
	HBRUSH hBrush = CreateSolidBrush(StickmanPara.color);
	SelectObject(hdc, hBrush);
	int state = Player[id].state;
	if (gameState != STARTED) return;
	POINT point = WorldSetMapper(hdc, Player[id].x, Player[id].y);
	getPoint(x, y, StickmanPara.lBody, StickManState[state].aBody, &ux, &uy);
	getPoint(ux, uy, StickmanPara.lHead, StickManState[state].aHead, &nx, &ny);
	Ellipse(hdc, nx - StickmanPara.rHead, ny - StickmanPara.rHead, nx + StickmanPara.rHead, ny + StickmanPara.rHead);
	DrawStick(hdc, x, y, ux, uy, StickmanPara.rStick);
	getPoint(ux, uy, StickmanPara.lUpperarm, StickManState[state].aUpperarm1, &nx, &ny);
	DrawStick(hdc, ux, uy, nx, ny, StickmanPara.rStick);
	getPoint(nx, ny, StickmanPara.lForearm, StickManState[state].aForearm1, &dx, &dy);
	DrawStick(hdc, nx, ny, dx, dy, StickmanPara.rStick);
	getPoint(ux, uy, StickmanPara.lUpperarm, StickManState[state].aUpperarm2, &nx, &ny);
	DrawStick(hdc, ux, uy, nx, ny, StickmanPara.rStick);
	getPoint(nx, ny, StickmanPara.lForearm, StickManState[state].aForearm2, &dx, &dy);
	DrawStick(hdc, nx, ny, dx, dy, StickmanPara.rStick);
	getPoint(x, y, StickmanPara.lThigh, StickManState[state].aThigh1, &nx, &ny);
	DrawStick(hdc, x, y, nx, ny, StickmanPara.rStick);
	getPoint(nx, ny, StickmanPara.lShank, StickManState[state].aShank1, &dx, &dy);
	DrawStick(hdc, nx, ny, dx, dy, StickmanPara.rStick);
	getPoint(dx, dy, StickmanPara.lFoot, StickManState[state].aFoot1, &nx, &ny);
	DrawStick(hdc, nx, ny, dx, dy, StickmanPara.rStick);
	getPoint(x, y, StickmanPara.lThigh, StickManState[state].aThigh2, &nx, &ny);
	DrawStick(hdc, x, y, nx, ny, StickmanPara.rStick);
	getPoint(nx, ny, StickmanPara.lShank, StickManState[state].aShank2, &dx, &dy);
	DrawStick(hdc, nx, ny, dx, dy, StickmanPara.rStick);
	getPoint(dx, dy, StickmanPara.lFoot, StickManState[state].aFoot2, &nx, &ny);
	DrawStick(hdc, nx, ny, dx, dy, StickmanPara.rStick);
	WorldResetMapper(hdc, &point);
	DeleteObject(hBrush);
}

void StickmanTime(int id, int ms)
{
	Player[id].state = (Player[id].state + 8) % 9;
	TimerAdd(StickmanTime, 0, ms + 20);
}

void PlayerTracker(int id, double *x, double *y)
{
	*x = Player[id].x;
	*y = Player[id].y;
}


void PlayerInit()
{
	Player[0].x = 300;
	Player[0].y = 300;
	Player[0].state = 0;
	DrawerAdd(StickmanDraw, 0, 0);
	TimerAdd(StickmanTime, 0, 20);
	//WorldSetTracked(PlayerTracker, 0);
}
void PlayerDestory() {}
