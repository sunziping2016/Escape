#include <windows.h>
#include <tchar.h>

#include "resource.h"

#include "keyboard.h"
#include "drawer.h"
#include "timer.h"
#include "engine.h"
#include "log.h"

#define MAX_STRLEN		100
#define MIN_WINHEIGHT	640
#define MIN_WINWIDTH	960

static TCHAR szWindowClass[MAX_STRLEN];
static TCHAR szTitle[MAX_STRLEN];

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	MSG msg;
	WNDCLASSEX wcex = {
		sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW,
		WndProc, 0, 0, hInstance,
		LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON)),
		LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		NULL, szWindowClass,
		LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APP_ICON)) };
	LogInit();
	LoadString(hInstance, IDS_WINDOWCLASS, szWindowClass, MAX_STRLEN);
	LoadString(hInstance, IDS_APPTITLE, szTitle, MAX_STRLEN);
	if (!RegisterClassEx(&wcex)) {
		MessageBox(NULL, TEXT("Call to RegisterClassEx failed!"), szTitle, MB_OK);
		return EXIT_FAILURE;
	}
	hWnd = CreateWindow(
		szWindowClass, szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);
	if (!hWnd) {
		MessageBox(NULL, TEXT("Call to CreateWindow failed!"), szTitle, MB_OK);
		return EXIT_FAILURE;
	}
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	LogDestroy();
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT ps;
	LPMINMAXINFO lpMMI;
	switch (message)
	{
	case WM_CREATE:
		KeyboardInit();
		DrawerInit(hWnd);
		TimerInit();
		EngineInit();
		TimerProcess(hWnd);
		EngineProcess();
		break;
	case WM_SIZE:
		DrawerUpdateSize(hWnd);
		break;
	case WM_GETMINMAXINFO:
		lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = MIN_WINWIDTH;
		lpMMI->ptMinTrackSize.y = MIN_WINHEIGHT;
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
		EngineProcess();
		KeyboardClear();
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_DESTROY:
		EngineDestroy();
		TimerDestroy();
		DrawerDestroy();
		KeyboardDestoy();
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
