#include "main.h"
#include "manager.h"
#include <thread>

#include "input.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include <DirectXMath.h>
using namespace DirectX;

#pragma comment(lib, "winmm.lib")

#if _DEBUG
#pragma comment(lib, "DirectXTex_Debug.lib")
#else
#pragma comment(lib, "DirectXTex_Release.lib")
#endif

const wchar_t* CLASS_NAME = L"ZickyyMagnetBetaWindow";
const wchar_t* WINDOW_NAME = L"ZICKYY MAGNET BETA";

#ifdef _DEBUG
int g_CountFPS = 0;
#endif

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWND g_Window;

void ThrowIfFailed(HRESULT Result, const char* Operation)
{
	if (FAILED(Result))
	{
		char message[256];
		sprintf_s(message, "%s failed (HRESULT=0x%08X)", Operation, static_cast<unsigned int>(Result));
		throw std::runtime_error(message);
	}
}

void ThrowLastError(const char* Operation)
{
	const DWORD error = GetLastError();
	ThrowIfFailed(
		error == ERROR_SUCCESS ? E_FAIL : HRESULT_FROM_WIN32(error),
		Operation);
}

HWND GetWindow()
{
	return g_Window;
}

int APIENTRY WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE,
	_In_ LPSTR commandLine,
	_In_ int nCmdShow)
{
	DWORD dwExecLastTime;
	DWORD dwFPSLastTime;
	DWORD dwCurrentTime;
	DWORD dwFrameCount;

	WNDCLASSEXW wcex{};
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.lpszClassName = CLASS_NAME;

	bool windowClassRegistered = false;
	bool comInitialized = false;
	bool managerStarted = false;

	try
	{
		if (!std::filesystem::exists("asset/model"))
		{
			wchar_t executable[MAX_PATH]{};
			const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
			if (length > 0 && length < MAX_PATH)
			{
				auto directory = std::filesystem::path(executable).parent_path();
				for (int i = 0; i < 4 && !directory.empty(); ++i)
				{
					if (std::filesystem::exists(directory / "asset/model") &&
						std::filesystem::exists(directory / "shader/unlitTextureVS.cso"))
					{
						std::filesystem::current_path(directory);
						break;
					}
					directory = directory.parent_path();
				}
			}
		}
		if (RegisterClassExW(&wcex) == 0)
		{
			ThrowLastError("RegisterClassEx");
		}
		windowClassRegistered = true;

		RECT rc = { 0, 0, static_cast<LONG>(SCREEN_WIDTH), static_cast<LONG>(SCREEN_HEIGHT) };
		if (!AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE))
		{
			ThrowLastError("AdjustWindowRect");
		}

		g_Window = CreateWindowExW(
			0,
			CLASS_NAME,
			WINDOW_NAME,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			rc.right - rc.left,
			rc.bottom - rc.top,
			nullptr,
			nullptr,
			hInstance,
			nullptr);
		if (g_Window == nullptr)
		{
			ThrowLastError("CreateWindowEx");
		}

		ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED), "CoInitializeEx");
		comInitialized = true;

		managerStarted = true;
		const bool startInGame =
			commandLine == nullptr ||
			strstr(commandLine, "--title") == nullptr;
		Manager::Init(startInGame);

		ShowWindow(g_Window, nCmdShow);
		UpdateWindow(g_Window);

		timeBeginPeriod(1);
		dwExecLastTime = dwFPSLastTime = timeGetTime();
		dwCurrentTime = dwFrameCount = 0;

		MSG msg{};
		bool running = true;
		while (running)
		{
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					running = false;
					break;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (!running)
			{
				break;
			}

			dwCurrentTime = timeGetTime();

			if ((dwCurrentTime - dwFPSLastTime) >= 1000)
			{
#ifdef _DEBUG
				g_CountFPS = dwFrameCount;
#endif
				dwFPSLastTime = dwCurrentTime;
				dwFrameCount = 0;
			}

			if ((dwCurrentTime - dwExecLastTime) >= (1000 / 60))
			{
				dwExecLastTime = dwCurrentTime;

#ifdef _DEBUG
				wchar_t debugStr[256];
				wsprintfW(debugStr, L"%s FPS:%d", WINDOW_NAME, g_CountFPS);
				SetWindowTextW(g_Window, debugStr);
#endif

				Manager::Update(1.0f / 60.0f);
				Manager::Draw(1.0f);

				dwFrameCount++;
			}
		}

		timeEndPeriod(1);

		Manager::Uninit();
		managerStarted = false;
		CoUninitialize();
		comInitialized = false;
		UnregisterClassW(CLASS_NAME, hInstance);
		windowClassRegistered = false;
		return static_cast<int>(msg.wParam);
	}
	catch (const std::exception& error)
	{
		if (managerStarted)
		{
			Manager::Uninit();
		}
		if (comInitialized)
		{
			CoUninitialize();
		}
		if (windowClassRegistered)
		{
			UnregisterClassW(CLASS_NAME, hInstance);
		}

		MessageBoxA(nullptr, error.what(), "GM_31 error", MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INPUT:
		Input::ProcessRawInput(lParam);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		Input::ProcessKeyDown(wParam, lParam);
		switch (wParam)
		{
		case VK_ESCAPE:
			DestroyWindow(hWnd);
			break;
		}
		break;

	default:
		break;
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}