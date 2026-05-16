#include "core/includes.h"
#include "core/vector.h"
#include "core/entity.h"
#include "core/config.h"
#include "core/cs2_runtime.h"
#include "features/esp.h"
#include "features/aimbot.h"
#include "features/createmove.h"
#include "features/triggerbot.h"
#include "features/visuals.h"
#include "features/menu.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HMODULE g_hModule = nullptr;

// Silent crash handler — no disk writes
static LONG WINAPI CrashHandler(PEXCEPTION_POINTERS info) {
    return EXCEPTION_EXECUTE_HANDLER;
}

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

bool menuOpen = false;
bool g_UnloadRequested = false;

// Global engine client pointer (shared by createmove.h and cs2_runtime.h)
void* g_EngineClient = nullptr;

CheatStatus g_Status;

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    if (menuOpen) {
        switch (uMsg) {
        case WM_KEYDOWN: case WM_KEYUP:
        case WM_SYSKEYDOWN: case WM_SYSKEYUP:
        case WM_LBUTTONDOWN: case WM_LBUTTONUP:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP:
        case WM_MBUTTONDOWN: case WM_MBUTTONUP:
        case WM_XBUTTONDOWN: case WM_XBUTTONUP:
            return true;
        }
    }

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

void InitImGui() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    io.IniFilename = nullptr;
    ApplyStyle();
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);
}

DWORD WINAPI CleanupThread(LPVOID) {
    while (!g_UnloadRequested)
        Sleep(10);

    Sleep(200);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (window && oWndProc)
        SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);

    if (mainRenderTargetView) {
        mainRenderTargetView->Release();
        mainRenderTargetView = nullptr;
    }
    if (pContext) {
        pContext->Release();
        pContext = nullptr;
    }
    if (pDevice) {
        pDevice->Release();
        pDevice = nullptr;
    }

    kiero::shutdown();
    HookRemove(&g_CMHook);
    ExitThread(0);
    return 0;
}

void RequestUnload() {
    SaveConfig();
    g_UnloadRequested = true;
    HANDLE hThread = CreateThread(nullptr, 0, CleanupThread, nullptr, 0, nullptr);
    if (hThread) CloseHandle(hThread);
}

void RenderWatermark() {
    auto* list = ImGui::GetBackgroundDrawList();
    float fps = ImGui::GetIO().Framerate;
    static float ping = 0.f;

    __try {
        HMODULE engine = GetModuleHandleA(X("engine2.dll"));
        if (engine) {
            uintptr_t netClient = *(uintptr_t*)((uintptr_t)engine + 0x9090C0);
            if (netClient && netClient > 0x100000)
                ping = *(float*)(netClient + 0x280) * 1000.f;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    int sw = GetSystemMetrics(SM_CXSCREEN);
    char name[] = "CE";
    char fpsStr[16], pingStr[16];
    sprintf_s(fpsStr, "%d FPS", (int)fps);
    sprintf_s(pingStr, "%d ms", (int)ping);

    ImVec2 sName = ImGui::CalcTextSize(name);
    ImVec2 sFps  = ImGui::CalcTextSize(fpsStr);
    ImVec2 sPing = ImGui::CalcTextSize(pingStr);

    float padX = 12, padY = 6, gap = 14;
    float boxX = padX * 2 + sName.x + gap + sFps.x + gap + sPing.x;
    float boxY = padY * 2 + sName.y;
    float bx = (float)sw - boxX - 8;
    float by = 8;

    list->AddRectFilled(ImVec2(bx, by), ImVec2(bx + boxX, by + boxY), IM_COL32(7, 7, 9, 210), 4);
    list->AddRect(ImVec2(bx, by), ImVec2(bx + boxX, by + boxY), IM_COL32(255, 140, 0, 200), 4);

    ImColor orange(1.f, 0.55f, 0.f, 0.9f);
    float cx = bx + padX;
    list->AddText(ImVec2(cx, by + padY), orange, name);
    cx += sName.x + gap;
    list->AddText(ImVec2(cx, by + padY), orange, fpsStr);
    cx += sFps.x + gap;
    list->AddText(ImVec2(cx, by + padY), orange, pingStr);
}

bool init = false;

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (g_UnloadRequested)
        return oPresent(pSwapChain, SyncInterval, Flags);

    if (!init) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice))) {
            pDevice->GetImmediateContext(&pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
            InitImGui();
            InitConfigPath(g_hModule);
            LoadConfig();
            init = true;
        } else {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
    }

    static int frameCount = 0;
    frameCount++;

    if (GetAsyncKeyState(VK_INSERT) & 1) {
        menuOpen = !menuOpen;
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = menuOpen;
        if (!menuOpen) SaveConfig();
    }

    if (GetAsyncKeyState(VK_END) & 1)
        RequestUnload();

    // Re-enable entity updates for aimbot/bhop
    __try { UpdateEntities(); } __except(EXCEPTION_EXECUTE_HANDLER) {}

    // D3D11 frame setup - now uses dynamic D3DCompile loading
    ImGui_ImplDX11_NewFrame();

    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    if (menuOpen && window) {
        RECT r;
        if (GetWindowRect(window, &r))
            ClipCursor(&r);
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(window, &pt);
        io.MousePos = ImVec2((float)pt.x, (float)pt.y);
        io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        io.MouseDown[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    }

    if (menuOpen)
        RenderMenu(menuOpen);

    if (settings::watermark && !menuOpen)
        RenderWatermark();

    if (settings::triggerbotEnabled)
        RunTriggerbot();

    if (settings::fovChanger || settings::noFlash || settings::noFog)
        ApplyVisuals();

    if (settings::espEnabled && (settings::box || settings::boxCorner || settings::health || settings::name || settings::distance))
        DrawESP();

    if (settings::aimbotShowFov)
        DrawFOV();

    ImGui::Render();
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}

void InitProtection() {
    HWND gameWnd = nullptr;
    while (!gameWnd) {
        gameWnd = GetForegroundWindow();
        if (gameWnd) break;
        Sleep(50);
    }
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    g_hModule = (HMODULE)lpParam;

    InitProtection();

    HMODULE client = nullptr;
    while (!client) {
        client = GetModuleHandleA(X("client.dll"));
        Sleep(50);
    }

    g_Status.clientBase = (uintptr_t)client;
    sprintf_s(g_Status.clientDll, "0x%llX", (uintptr_t)client);

    InitRuntime();

    g_Status.entityList = g_Offsets.entityList;
    g_Status.viewAngles = g_Offsets.viewAngles;
    g_Status.viewMatrix = g_Offsets.viewMatrix;

    bool init_hook = false;
    do {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success) {
            kiero::bind(8, (void**)&oPresent, hkPresent);
            g_Status.presentHooked = true;
            init_hook = true;
        } else {
            Sleep(100);
        }
    } while (!init_hook);

    if (InitCreateMoveHook(&g_Status.createMoveAddr)) {
        g_Status.createMoveHooked = true;
        sprintf_s(g_Status.cmRva, "0x%llX", g_Status.createMoveAddr - (uintptr_t)client);
    }

    g_Status.initialized = true;
    return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        g_hModule = hMod;
        DisableThreadLibraryCalls(hMod);
        SetUnhandledExceptionFilter(CrashHandler);
        CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        kiero::shutdown();
        break;
    }
    return TRUE;
}
