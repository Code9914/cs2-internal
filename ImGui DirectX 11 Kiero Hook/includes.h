#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <d3d11.h>
#include <dxgi.h>
#include "kiero/kiero.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "game_offsets.h"

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;
using CreateInterfaceFn = void* (__stdcall*)(const char* name, int* returnCode);

namespace settings {
    // --- ESP ---
    inline bool   espEnabled     = true;
    inline bool   espTeamCheck   = true;
    inline bool   box            = true;
    inline bool   boxFilled      = false;
    inline bool   boxCorner      = false;
    inline float  boxThickness  = 2.f;
    inline float  boxColor[4]   = { 1.f, 0.55f, 0.f, 1.f }; // orange
    inline float  boxFilledColor[4] = { 0.f, 0.f, 0.f, 0.3f };

    inline bool   health         = true;
    inline float  healthColor[4] = { 0.f, 1.f, 0.f, 1.f };

    inline bool   name           = true;
    inline float  nameColor[4]   = { 1.f, 1.f, 1.f, 1.f };

    inline bool   distance       = false;
    inline float  distColor[4]  = { 1.f, 1.f, 1.f, 1.f };

    // --- Misc ---
    inline bool   watermark     = true;
}
