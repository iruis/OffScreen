#include <Windows.h>
#include <tchar.h>

#include <gl/GL.h>
#include <gl/GLU.h>

#include "wglext.h"

#pragma comment(lib, "Opengl32.lib")

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

static TCHAR* lpszClass = TEXT("TEST WINDOW");
static LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

// https://suzulang.com/opengl-dib-section-offscreenrendering/

struct rgb_t
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

static const struct rgb_t Black = { .r = 0, .g = 0, .b = 0 };
static const struct rgb_t White = { .r = 255, .g = 255, .b = 255 };

static struct rgb_t MakeRGB(unsigned char r, unsigned char g, unsigned char b)
{
	struct rgb_t rgb =
	{
		.r = r,
		.g = g,
		.b = b,
	};
	return rgb;
}

void CreateDIBSection24(HBITMAP* hBitmap, HDC* hMemDC, BITMAPINFO* bmpInfo, struct rgb_t** lpPixel, LONG width, LONG height)
{
	bmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo->bmiHeader.biWidth = width;
	bmpInfo->bmiHeader.biHeight = height;
	bmpInfo->bmiHeader.biPlanes = 1;
	bmpInfo->bmiHeader.biBitCount = 24;
	bmpInfo->bmiHeader.biCompression = BI_RGB;

	HDC hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

	*hBitmap = CreateDIBSection(hdc, bmpInfo, DIB_RGB_COLORS, lpPixel, NULL, 0);
	*hMemDC = CreateCompatibleDC(hdc);

	DeleteDC(hdc);

	SelectObject(*hMemDC, *hBitmap);
}

void DeleteDIBSection32(HDC hMemDC, HBITMAP hBitmap)
{
	DeleteDC(hMemDC);
	DeleteObject(hBitmap);
}

void InitializeGL(HGLRC* hRC, HDC hdc)
{
	PIXELFORMATDESCRIPTOR pfd =
	{
		.nSize = sizeof(PIXELFORMATDESCRIPTOR),
		.nVersion = 1,
		.dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 24,
		.cRedBits = 0,
		.cRedShift = 0,
		.cGreenBits = 0,
		.cGreenShift = 0,
		.cBlueBits = 0,
		.cBlueShift = 0,
		.cAlphaBits = 0,
		.cAlphaShift = 0,
		.cAccumBits = 0,
		.cAccumRedBits = 0,
		.cAccumGreenBits = 0,
		.cAccumBlueBits = 0,
		.cAccumAlphaBits = 0,
		.cDepthBits = 32,
		.cStencilBits = 0,
		.cAuxBuffers = 0,
		.iLayerType = PFD_MAIN_PLANE,
		.bReserved = 0,
		.dwLayerMask = 0,
		.dwVisibleMask = 0,
		.dwDamageMask = 0
	};

	SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd);

	*hRC = wglCreateContext(hdc);

	wglMakeCurrent(hdc, *hRC);
}

void DeInitializeGL(HGLRC hRC, HDC hdc)
{
	wglMakeCurrent(hdc, NULL);
	wglDeleteContext(hRC);
}

struct CustomData
{
	HMODULE hOpenGL;

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

	HDC hMemDC;
	HGLRC hRC;
	HBITMAP hBitmap;
	BITMAPINFO bmpInfo;
	struct rgb_t* lpPixel;
	float rotate;
	long width;
	long height;

	long time;
	long count;
	long fps;
};


void Draw(struct CustomData* customData)
{
	wglMakeCurrent(customData->hMemDC, customData->hRC);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glLoadIdentity();
	glRotatef(customData->rotate, 0.0f, 1.0f, 0.0f);

	glBegin(GL_QUADS);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(-0.7f, -0.7f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(-0.7f, 0.7f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.7f, 0.7f, 0.0f);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex3f(0.7f, -0.7f, 0.0f);
	glEnd();

	glFlush();

	SwapBuffers(customData->hMemDC);
	wglMakeCurrent(customData->hMemDC, NULL);

	customData->rotate += 10.0f;
	customData->count++;
}

static LRESULT CALLBACK FakeProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}

static BOOL InitializeGLFunctions(struct CustomData* customData)
{
	BOOL success = FALSE;
	PIXELFORMATDESCRIPTOR fakePFD =
	{
		.nSize = sizeof(PIXELFORMATDESCRIPTOR),
		.nVersion = 1,
		.dwFlags = PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_GENERIC_ACCELERATED | PFD_DOUBLEBUFFER | PFD_SWAP_LAYER_BUFFERS,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 32,
		.cRedBits = 8,
		.cRedShift = 0,
		.cGreenBits = 8,
		.cGreenShift = 0,
		.cBlueBits = 8,
		.cBlueShift = 0,
		.cAlphaBits = 8,
		.cAlphaShift = 0,
		.cAccumBits = 0,
		.cAccumRedBits = 0,
		.cAccumGreenBits = 0,
		.cAccumBlueBits = 0,
		.cAccumAlphaBits = 0,
		.cDepthBits = 32,
		.cStencilBits = 8,
		.cAuxBuffers = 0,
		.iLayerType = PFD_MAIN_PLANE,
		.bReserved = 0,
		.dwLayerMask = 0,
		.dwVisibleMask = 0,
		.dwDamageMask = 0
	};
	WNDCLASSEX wndClassEx =
	{
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
		.lpfnWndProc = FakeProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = HINST_THISCOMPONENT,
		.hIcon = NULL,
		.hCursor = NULL,
		.hbrBackground = NULL,
		.lpszMenuName = NULL,
		.lpszClassName = TEXT("FAKE"),
		.hIconSm = NULL
	};
	RegisterClassEx(&wndClassEx);

	HWND fakeWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, wndClassEx.lpszClassName, wndClassEx.lpszClassName, WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, HINST_THISCOMPONENT, NULL);
	HDC fakeDC = GetDC(fakeWnd);

	//ShowWindow(fakeWnd, SW_SHOW);

	int fakePFDID = ChoosePixelFormat(fakeDC, &fakePFD);
	if (fakePFDID != 0)
	{
		if (SetPixelFormat(fakeDC, fakePFDID, &fakePFD) == TRUE)
		{
			HGLRC fakeRC = wglCreateContext(fakeDC);
			if (fakeRC != 0)
			{
				wglMakeCurrent(fakeDC, fakeRC);

				customData->wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
				customData->wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

				if (customData->wglChoosePixelFormatARB == NULL || customData->wglCreateContextAttribsARB == NULL)
				{
					customData->hOpenGL = LoadLibrary(TEXT("opengl32.dll"));
					if (customData->hOpenGL != INVALID_HANDLE_VALUE)
					{
						customData->wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)GetProcAddress(customData->hOpenGL, "wglChoosePixelFormatARB");
						customData->wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GetProcAddress(customData->hOpenGL, "wglCreateContextAttribsARB");
					}
				}

				success = TRUE;
			}
		}
	}

	UnregisterClass(wndClassEx.lpszClassName, HINST_THISCOMPONENT);

	return success;
}

int APIENTRY _tWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR lpszCmdParam,
	_In_ int nCmdShow)
{
	const long width = 800;
	const long height = 600;

	RECT windowRect;
	windowRect.left = 0;
	windowRect.right = width;
	windowRect.top = 0;
	windowRect.bottom = height;

	WNDCLASSEX wndClassEx;
	wndClassEx.cbSize = sizeof(WNDCLASSEX);
	wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
	wndClassEx.lpfnWndProc = (WNDPROC)WndProc;
	wndClassEx.cbClsExtra = 0;
	wndClassEx.cbWndExtra = sizeof(LONG_PTR);
	wndClassEx.hInstance = HINST_THISCOMPONENT;
	wndClassEx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClassEx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndClassEx.lpszMenuName = NULL;
	wndClassEx.lpszClassName = lpszClass;
	wndClassEx.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	if (!RegisterClassEx(&wndClassEx))
	{
		return 0;
	}
	AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);

	struct CustomData* customData = malloc(sizeof(struct CustomData));
	if (customData == NULL)
	{
		return 0;
	}
	ZeroMemory(customData, sizeof(struct CustomData));
	customData->width = 480;
	customData->height = 480;

	InitializeGLFunctions(customData);

	CreateDIBSection24(&customData->hBitmap, &customData->hMemDC, &customData->bmpInfo, &customData->lpPixel, customData->width, customData->height);
	InitializeGL(&customData->hRC, customData->hMemDC);

	HWND hwnd = CreateWindowEx(
		0,
		lpszClass,
		TEXT("OpenGL (WGL)"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		(HMENU)NULL,
		HINST_THISCOMPONENT,
		customData);

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	MSG msg;
	BOOL done = FALSE;
	ZeroMemory(&msg, sizeof(MSG));
	ULONGLONG lastTick = GetTickCount64();

	while (done == FALSE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				done = TRUE;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			ULONGLONG currentTick = GetTickCount64();
			if (currentTick - lastTick > 15)
			{
				Draw(customData);
				InvalidateRect(hwnd, NULL, FALSE);

				lastTick += 15;
			}
		}
	}

	DeInitializeGL(customData->hRC, customData->hMemDC);
	DeleteDIBSection32(customData->hMemDC, customData->hBitmap);

	if (customData->hOpenGL)
	{
		FreeLibrary(customData->hOpenGL);
	}

	free(customData);

	return msg.wParam;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	struct CustomData* customData;

	if (message == WM_CREATE)
	{
		LPCREATESTRUCT createStruct = (LPCREATESTRUCT)lParam;

		customData = (struct CustomData*)createStruct->lpCreateParams;

		SetWindowLongPtr(hwnd, GWLP_USERDATA, (ULONG_PTR)customData);

		return 1;
	}
	else
	{
		customData = (struct CustomData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

		if (customData)
		{
			switch (message)
			{
			case WM_PAINT:
			{
				SYSTEMTIME st;
				GetSystemTime(&st);

				if (customData->time != st.wSecond)
				{
					customData->time = st.wSecond;
					customData->fps = customData->count;
					customData->count = 0;
				}
				TCHAR fps[120];
				_stprintf_s(fps, sizeof(fps) / sizeof(fps[0]), TEXT("%d"), customData->fps);

				PAINTSTRUCT ps;
				HDC hDC = BeginPaint(hwnd, &ps);

				BitBlt(hDC, 0, 0, customData->width, customData->height, customData->hMemDC, 0, 0, SRCCOPY);
				TextOut(hDC, 20, 20, fps, _tcslen(fps));
				EndPaint(hwnd, &ps);

				return 0;
			}
			case WM_DESTROY:
				PostQuitMessage(0);

				return 0;

				//case WM_SIZE:
				//	width = LOWORD(lParam);
				//	height = HIWORD(lParam);

				//	return 0;
			}
		}
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
