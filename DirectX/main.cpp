

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

//#include "DirectXTex.h"

#if _DEBUG
#pragma comment(lib, "DirectXTex_Debug.lib")
#else
#pragma comment(lib, "DirectXTex_Release.lib")
#endif

const wchar_t* CLASS_NAME = L"ZickyyMagnetBetaWindow";
const wchar_t* WINDOW_NAME = L"ZICKYY MAGNET BETA";


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
		// Allow launching x64/Release/GM_31.exe directly from Explorer.
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
		// Start the magnetic beta immediately. The old runner title remains
		// available for regression checks with the --title argument.
		const bool startInGame =
			commandLine == nullptr ||
			strstr(commandLine, "--title") == nullptr;
		Manager::Init(startInGame);

		ShowWindow(g_Window, nCmdShow);
		UpdateWindow(g_Window);

		using Clock = std::chrono::steady_clock;
		constexpr double FixedStep = 1.0 / 60.0;
		constexpr double MaxFrameTime = 0.25;
		auto previousTime = Clock::now();
		double accumulator = 0.0;

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

			const auto currentTime = Clock::now();
			const double frameTime = std::min(
				std::chrono::duration<double>(currentTime - previousTime).count(),
				MaxFrameTime);
			previousTime = currentTime;
			accumulator += frameTime;

			while (accumulator >= FixedStep)
			{
				Manager::Update(static_cast<float>(FixedStep));
				accumulator -= FixedStep;
			}

			Manager::Draw(static_cast<float>(accumulator / FixedStep));
		}

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

