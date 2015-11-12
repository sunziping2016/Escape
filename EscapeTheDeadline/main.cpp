#include <windows.h>
#include <tchar.h>

#include "keyboard.h"
#include "drawer.h"
#include "timer.h"

#include "resource.h"

struct {
	int x, y;
	int ms;
} mS[10];

HBRUSH hb = CreateSolidBrush(0);

void DrawMS(HDC hDC, int id)
{
	RECT rect = { mS[id].x, mS[id].y, mS[id].x + 10, mS[id].y + 10 };
	FillRect(hDC, &rect, hb);
}
void TimerMS(int id, int ms)
{
	mS[id].x = (mS[id].x - 50 + 1) % 1000 + 50;
	TimerAdd(TimerMS, id, mS[id].ms + ms);
}
void InitMS()
{
	int i = 0;
	for (i = 0; i < 10; ++i) {
		mS[i].x = 50;
		mS[i].y = 20 * i + 20;
		mS[i].ms = i * 5 + 10;
		DrawerAdd(DrawMS, i, 0);
		TimerAdd(TimerMS, i, mS[i].ms);
	}
}


static TCHAR szWindowClass[MAX_STRLEN];
static TCHAR szTitle[MAX_STRLEN];

enum GameState gameState;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wcex;
	HWND hWnd;
	MSG msg;

	LoadString(hInstance, IDS_WINDOWCLASS, szWindowClass, MAX_STRLEN);
	LoadString(hInstance, IDS_APPTITLE, szTitle, MAX_STRLEN);

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APP_ICON));

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, TEXT("Call to RegisterClassEx failed!"), szTitle, MB_OK);
		return EXIT_FAILURE;
	}

	hWnd = CreateWindow(
		szWindowClass, szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		MessageBox(NULL, TEXT("Call to CreateWindow failed!"), szTitle, MB_OK);
		return EXIT_FAILURE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT ps;
	switch (message)
	{
	case WM_CREATE:
		KeyboardInit();
		DrawerInit(hWnd);
		TimerInit();
		InitMS();
		TimerProcess(hWnd);
		break;
	case WM_SIZE:
		DrawerUpdateSize(hWnd);
		break;
	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);
		DrawerProcess(hDC);
		EndPaint(hWnd, &ps);
		break;
	case WM_KEYDOWN:
		KeyboardKeyDown(wParam);
		break;
	case WM_KEYUP:
		KeyboardKeyUp(wParam);
		break;
	case WM_LBUTTONDOWN:
		break;
	case WM_TIMER:
		TimerProcess(hWnd);
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_DESTROY:
		TimerDestroy();
		DrawerDestroy();
		KeyboardDestoy();
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
