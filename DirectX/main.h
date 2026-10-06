#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#define NOMINMAX
#include <windows.h>
#include <assert.h>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")


#include <DirectXMath.h>
using namespace DirectX;


#include "DirectXTex.h"

#include "vector3.h"
#include <list>
#include <vector>

//#pragma comment (lib, "win mm.lib")


#define SCREEN_WIDTH	(1280)
#define SCREEN_HEIGHT	(720)


HWND GetWindow();

void ThrowIfFailed(HRESULT Result, const char* Operation);

template <typename T>
void SafeRelease(T*& Resource)
{
	if (Resource != nullptr)
	{
		Resource->Release();
		Resource = nullptr;
	}
}

void Invoke(std::function<void()> Function, int Time);

