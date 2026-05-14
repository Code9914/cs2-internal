#include "core/includes.h"
#include "core/vector.h"
#include "core/entity.h"
#include "features/esp.h"
#include "core/cs2_runtime.h"
#include "features/aimbot.h"
#include "features/createmove.h"



extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

bool menuOpen = false;
bool g_UnloadRequested = false;
HMODULE g_hModule = nullptr;

char g_ConfigPath[MAX_PATH] = {};

void LoadConfig() {
    FILE* f = nullptr;
    if (fopen_s(&f, g_ConfigPath, "rb") || !f) return;

    int b = 1, bf = 0, bc = 0, hp = 1, nm = 1, dist = 0, esp = 1, et = 1, wm = 1;
    int ae = 0, tc = 1, sf = 0, te = 0, tk = 0, se = 0;
    int ak = VK_LBUTTON, tk2 = VK_MENU, ah = 0, nf = 0, fc = 0, nfg = 0;
    float bt = 2.f, fov = 8.f, sm = 6.f, fv = 90.f;
    float bcol[4]={1,0.55f,0,1}, bfill[4]={0,0,0,0.3f};
    float hcol[4]={0,1,0,1}, ncol[4]={1,1,1,1}, dcol[4]={1,1,1,1};

    fscanf_s(f, "%d %d %d %d %d %d %d %d %d"
        " %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f"
        " %d %d %d %d %d %f %f %d"
        " %d %d %d %f"
        " %d %d %d",
        &b, &bf, &bc, &hp, &nm, &dist, &esp, &et, &wm,
        &bt, &bcol[0], &bcol[1], &bcol[2], &bcol[3],
        &bfill[0], &bfill[1], &bfill[2], &bfill[3],
        &hcol[0], &hcol[1], &hcol[2], &hcol[3],
        &ncol[0], &ncol[1], &ncol[2], &ncol[3],
        &dcol[0], &dcol[1], &dcol[2], &dcol[3],
        &ae, &tc, &sf, &te, &tk, &fov, &sm, &se,
        &ak, &tk2, &ah, &fv,
        &nf, &fc, &nfg);
    fclose(f);

    settings::box = b!=0; settings::boxFilled = bf!=0; settings::boxCorner = bc!=0;
    settings::health = hp!=0; settings::name = nm!=0;
    settings::distance = dist!=0; settings::espEnabled = esp!=0;
    settings::espTeamCheck = et!=0;
    settings::watermark = wm!=0;
    settings::aimbotEnabled = ae!=0; settings::aimbotTeamCheck = tc!=0;
    settings::aimbotShowFov = sf!=0;
    settings::triggerbotEnabled = te!=0; settings::triggerbotTeamCheck = tk!=0;
    settings::aimbotFov = fov; settings::aimbotSmooth = sm; settings::aimbotSmoothEnabled = se!=0;
    settings::aimbotKey = ak; settings::triggerbotKey = tk2;
    settings::aimbotHitbox = ah; settings::fovValue = fv;
    settings::noFlash = nf!=0; settings::fovChanger = fc!=0; settings::noFog = nfg!=0;
    settings::boxThickness = bt; memcpy(settings::boxColor, bcol, 16);
    memcpy(settings::boxFilledColor, bfill, 16); memcpy(settings::healthColor, hcol, 16);
    memcpy(settings::nameColor, ncol, 16); memcpy(settings::distColor, dcol, 16);
}

void SaveConfig() {
    FILE* f = nullptr;
    if (fopen_s(&f, g_ConfigPath, "wb") || !f) return;

    fprintf_s(f, "%d %d %d %d %d %d %d %d %d"
        " %.1f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f"
        " %d %d %d %d %d"
        " %f %f %d %d %d %d %.1f"
        " %d %d %d",
        settings::box, settings::boxFilled, settings::boxCorner,
        settings::health, settings::name, settings::distance, settings::espEnabled,
        settings::espTeamCheck,
        settings::watermark,
        settings::boxThickness,
        settings::boxColor[0], settings::boxColor[1], settings::boxColor[2], settings::boxColor[3],
        settings::boxFilledColor[0], settings::boxFilledColor[1], settings::boxFilledColor[2], settings::boxFilledColor[3],
        settings::healthColor[0], settings::healthColor[1], settings::healthColor[2], settings::healthColor[3],
        settings::nameColor[0], settings::nameColor[1], settings::nameColor[2], settings::nameColor[3],
        settings::distColor[0], settings::distColor[1], settings::distColor[2], settings::distColor[3],
        settings::aimbotEnabled, settings::aimbotTeamCheck, settings::aimbotShowFov,
        settings::triggerbotEnabled, settings::triggerbotTeamCheck,
        settings::aimbotFov, settings::aimbotSmooth, settings::aimbotSmoothEnabled,
        settings::aimbotKey, settings::triggerbotKey, settings::aimbotHitbox,
        settings::fovValue,
        settings::noFlash, settings::fovChanger, settings::noFog);
    fclose(f);
}

void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4 orange(1.f, 0.55f, 0.f, 1.f);
    ImVec4 darkBg(0.07f, 0.07f, 0.09f, 1.f);
    ImVec4 darkPanel(0.11f, 0.11f, 0.14f, 1.f);
    ImVec4 darkFrame(0.16f, 0.16f, 0.20f, 1.f);

    s.WindowRounding     = 6.f;
    s.ChildRounding      = 4.f;
    s.FrameRounding      = 4.f;
    s.PopupRounding      = 4.f;
    s.ScrollbarRounding  = 4.f;
    s.GrabRounding       = 4.f;
    s.TabRounding        = 4.f;

    s.WindowPadding    = ImVec2(14, 14);
    s.FramePadding     = ImVec2(10, 6);
    s.ItemSpacing      = ImVec2(10, 7);
    s.ItemInnerSpacing = ImVec2(8, 5);
    s.ScrollbarSize    = 10.f;
    s.GrabMinSize      = 10.f;
    s.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    auto& cl = s.Colors;
    cl[ImGuiCol_WindowBg]          = darkBg;
    cl[ImGuiCol_ChildBg]           = darkPanel;
    cl[ImGuiCol_PopupBg]           = darkPanel;
    cl[ImGuiCol_FrameBg]           = darkFrame;
    cl[ImGuiCol_FrameBgHovered]    = ImVec4(0.22f, 0.22f, 0.28f, 1.f);
    cl[ImGuiCol_FrameBgActive]     = orange;
    cl[ImGuiCol_TitleBg]           = darkPanel;
    cl[ImGuiCol_TitleBgActive]     = ImVec4(0.14f, 0.14f, 0.18f, 1.f);
    cl[ImGuiCol_TitleBgCollapsed]  = darkPanel;
    cl[ImGuiCol_MenuBarBg]         = darkPanel;
    cl[ImGuiCol_ScrollbarBg]       = darkPanel;
    cl[ImGuiCol_ScrollbarGrab]     = orange;
    cl[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.f, 0.65f, 0.1f, 1.f);
    cl[ImGuiCol_ScrollbarGrabActive]  = ImVec4(1.f, 0.7f, 0.2f, 1.f);
    cl[ImGuiCol_CheckMark]         = orange;
    cl[ImGuiCol_SliderGrab]        = orange;
    cl[ImGuiCol_SliderGrabActive]  = ImVec4(1.f, 0.65f, 0.1f, 1.f);
    cl[ImGuiCol_Button]            = darkFrame;
    cl[ImGuiCol_ButtonHovered]     = orange;
    cl[ImGuiCol_ButtonActive]      = ImVec4(1.f, 0.65f, 0.1f, 1.f);
    cl[ImGuiCol_Header]            = orange;
    cl[ImGuiCol_HeaderHovered]     = ImVec4(1.f, 0.65f, 0.1f, 1.f);
    cl[ImGuiCol_HeaderActive]      = ImVec4(1.f, 0.7f, 0.2f, 1.f);
    cl[ImGuiCol_Separator]         = orange;
    cl[ImGuiCol_Tab]               = darkPanel;
    cl[ImGuiCol_TabHovered]        = orange;
    cl[ImGuiCol_TabActive]         = orange;
    cl[ImGuiCol_TabUnfocused]      = darkPanel;
    cl[ImGuiCol_TabUnfocusedActive]= orange;
    cl[ImGuiCol_Text]              = ImVec4(0.92f, 0.92f, 0.94f, 1.f);
    cl[ImGuiCol_TextDisabled]      = ImVec4(0.45f, 0.45f, 0.50f, 1.f);
    cl[ImGuiCol_Border]           = ImVec4(0.18f, 0.18f, 0.22f, 1.f);
    cl[ImGuiCol_BorderShadow]     = ImVec4(0, 0, 0, 0);
}



void InitImGui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    ApplyStyle();
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);
}

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

DWORD WINAPI CleanupThread(LPVOID lpParam)
{
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

    FreeConsole();
    FreeLibraryAndExitThread(g_hModule, 0);
    return 0;
}

void RequestUnload()
{
    SaveConfig();
    g_UnloadRequested = true;
    HANDLE hThread = CreateThread(nullptr, 0, CleanupThread, nullptr, 0, nullptr);
    if (hThread)
        CloseHandle(hThread);
}





bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (g_UnloadRequested)
        return oPresent(pSwapChain, SyncInterval, Flags);

    if (!init)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
        {
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

            GetModuleFileNameA(g_hModule, g_ConfigPath, MAX_PATH);
            char* dot = strrchr(g_ConfigPath, '.');
            if (dot) strcpy_s(dot, 5, ".cfg");
            LoadConfig();

            init = true;
        }

        else
            return oPresent(pSwapChain, SyncInterval, Flags);
    }

    if (GetAsyncKeyState(VK_INSERT) & 1) {
        menuOpen = !menuOpen;
        ImGuiIO& io = ImGui::GetIO();
        if (menuOpen) {
            io.MouseDrawCursor = true;
        } else {
            io.MouseDrawCursor = false;
            SaveConfig();
        }
    }

    if (GetAsyncKeyState(VK_END) & 1)
        RequestUnload();

    __try { UpdateEntities(); } __except(EXCEPTION_EXECUTE_HANDLER) {}

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    if (menuOpen && window) {
        RECT r;
        if (GetWindowRect(window, &r)) {
            ClipCursor(&r);
        }
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(window, &pt);
        io.MousePos = ImVec2((float)pt.x, (float)pt.y);
        io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        io.MouseDown[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    }

    if (menuOpen) {
        ImGui::SetNextWindowSizeConstraints(ImVec2(550, 420), ImVec2(800, 600));
        ImGui::Begin("CockEngine", &menuOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::BeginTabBar("Tabs")) {

            // === Aimbot ===
            if (ImGui::BeginTabItem("Aimbot")) {
                if (ImGui::BeginChild("##aimbot_left", ImVec2(ImGui::GetWindowWidth() / 2 - 20, 0), true)) {
                    ImGui::TextColored(ImVec4(1,0.55f,0,1), "Aimbot");
                    ImGui::Separator();

                    ImGui::Checkbox("Enable", &settings::aimbotEnabled);
                    ImGui::Checkbox("Team check", &settings::aimbotTeamCheck);
                    ImGui::Combo("Hitbox", &settings::aimbotHitbox, "Head\0Neck\0Chest\0");

                    ImGui::Separator();
                    ImGui::TextDisabled("-- Key --");
                    if (settings::aimbotKeyWait) {
                        ImGui::Text("Press a key...");
                        for (int k = 1; k < 256; k++) {
                            if ((GetAsyncKeyState(k) & 1) && k != VK_INSERT && k != VK_END) {
                                settings::aimbotKey = k;
                                settings::aimbotKeyWait = false;
                                break;
                            }
                        }
                    } else {
                        if (ImGui::Button("Bind key")) settings::aimbotKeyWait = true;
                        ImGui::SameLine();
                        ImGui::Text("Key: %s", AimbotKeyName(settings::aimbotKey));
                    }

                    ImGui::Separator();
                    ImGui::TextDisabled("-- FOV --");
                    ImGui::Checkbox("Show circle", &settings::aimbotShowFov);
                    if (settings::aimbotShowFov)
                        ImGui::SliderFloat("Radius", &settings::aimbotFov, 0.5f, 30.f, "%.1f");

                    ImGui::Separator();
                    ImGui::TextDisabled("-- Smoothing --");
                    ImGui::Checkbox("Smooth", &settings::aimbotSmoothEnabled);
                    if (settings::aimbotSmoothEnabled)
                        ImGui::SliderFloat("Value", &settings::aimbotSmooth, 0.f, 30.f, "%.1f");
                }
                ImGui::EndChild();

                ImGui::SameLine();

                if (ImGui::BeginChild("##triggerbot_right", ImVec2(ImGui::GetWindowWidth() / 2 - 20, 0), true)) {
                    ImGui::TextColored(ImVec4(1,0.55f,0,1), "Triggerbot");
                    ImGui::Separator();

                    ImGui::Checkbox("Enable", &settings::triggerbotEnabled);
                    ImGui::Checkbox("Team check", &settings::triggerbotTeamCheck);

                    ImGui::Separator();
                    ImGui::TextDisabled("-- Key --");
                    if (settings::triggerbotKeyWait) {
                        ImGui::Text("Press a key...");
                        for (int k = 1; k < 256; k++) {
                            if ((GetAsyncKeyState(k) & 1) && k != VK_INSERT && k != VK_END) {
                                settings::triggerbotKey = k;
                                settings::triggerbotKeyWait = false;
                                break;
                            }
                        }
                    } else {
                        if (ImGui::Button("Bind key")) settings::triggerbotKeyWait = true;
                        ImGui::SameLine();
                        ImGui::Text("Key: %s", AimbotKeyName(settings::triggerbotKey));
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // === ESP ===
            if (ImGui::BeginTabItem("ESP")) {
                ImGui::Checkbox("Enable ESP", &settings::espEnabled);
                ImGui::SameLine();
                ImGui::Checkbox("Team check", &settings::espTeamCheck);
                ImGui::Separator();

                ImGui::TextDisabled("-- Box --");
                ImGui::Checkbox("Box", &settings::box);
                ImGui::SameLine();
                ImGui::Checkbox("Corner", &settings::boxCorner);
                ImGui::SameLine();
                ImGui::Checkbox("Filled", &settings::boxFilled);
                if (settings::box || settings::boxCorner) {
                    ImGui::SliderFloat("Thickness", &settings::boxThickness, 0.5f, 5.f, "%.1f");
                    ImGui::ColorEdit4("Box Color", settings::boxColor, ImGuiColorEditFlags_NoInputs);
                }
                if (settings::boxFilled)
                    ImGui::ColorEdit4("Fill Color", settings::boxFilledColor, ImGuiColorEditFlags_NoInputs);
                ImGui::Separator();

                ImGui::TextDisabled("-- Health --");
                ImGui::Checkbox("Health bar", &settings::health);
                if (settings::health)
                    ImGui::ColorEdit4("Health Color", settings::healthColor, ImGuiColorEditFlags_NoInputs);
                ImGui::Separator();

                ImGui::TextDisabled("-- Name --");
                ImGui::Checkbox("Name", &settings::name);
                if (settings::name)
                    ImGui::ColorEdit4("Name Color", settings::nameColor, ImGuiColorEditFlags_NoInputs);
                ImGui::Separator();

                ImGui::TextDisabled("-- Distance --");
                ImGui::Checkbox("Distance", &settings::distance);
                if (settings::distance)
                    ImGui::ColorEdit4("Dist. Color", settings::distColor, ImGuiColorEditFlags_NoInputs);

                ImGui::EndTabItem();
            }

            // === Visual ===
            if (ImGui::BeginTabItem("Visual")) {
                ImGui::Checkbox("No Flash", &settings::noFlash);
                ImGui::Separator();

                ImGui::Checkbox("FOV Changer", &settings::fovChanger);
                if (settings::fovChanger)
                    ImGui::SliderFloat("Value", &settings::fovValue, 60.f, 120.f, "%.0f");
                ImGui::Separator();

                ImGui::Checkbox("No Fog", &settings::noFog);

                ImGui::EndTabItem();
            }

            // === Misc ===
            if (ImGui::BeginTabItem("Misc")) {
                ImGui::TextDisabled("Hold space to auto-jump");
                ImGui::Separator();
                ImGui::Checkbox("Watermark", &settings::watermark);
                ImGui::EndTabItem();
            }

            // === Config ===
            if (ImGui::BeginTabItem("Config")) {
                ImGui::TextColored(ImVec4(1,0.55f,0,1), "Configuration");
                ImGui::Separator();

                if (ImGui::Button("Save Config", ImVec2(180, 0))) {
                    SaveConfig();
                }
                ImGui::SameLine();
                if (ImGui::Button("Load Config", ImVec2(180, 0))) {
                    LoadConfig();
                }

                ImGui::Separator();
        ImGui::TextDisabled("-- Keys --");
                ImGui::Text("INSERT  =  Toggle menu");
                ImGui::Text("END     =  Unload cheat");

                ImGui::EndTabItem();
            }

            // === About ===
            if (ImGui::BeginTabItem("About")) {
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
                ImGui::TextColored(ImVec4(1,0.55f,0,1), "CockEngine v1.0");
                ImGui::Separator();
                ImGui::TextDisabled("Internal cheat for Counter-Strike 2");
                ImGui::Separator();
                ImGui::BulletText("Interfaces via CreateInterface");
                ImGui::BulletText("SchemaSystem runtime resolution");
                ImGui::BulletText("Pattern scanning + RVA fallback");
                ImGui::BulletText("D3D11 Present hook (Kiero)");
                ImGui::Separator();
                ImGui::Text("Build: %s", __DATE__);
                ImGui::PopFont();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    __try { if (settings::aimbotShowFov) DrawFOV(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { if (settings::espEnabled) DrawESP(); } __except(EXCEPTION_EXECUTE_HANDLER) {}

    __try {
        if (settings::triggerbotEnabled) {
            uintptr_t localPawn = GetLocalPawn();
            if (localPawn && localPawn > 0x100000) {
                int idEntIndex = *(int*)(localPawn + 0x344C);
                if (idEntIndex > 0 && idEntIndex < 64) {
                    uintptr_t targetPawn = GetEntity(g_EntityList, idEntIndex);
                    if (targetPawn && targetPawn > 0x100000) {
                        uintptr_t targetCtrl = GetEntity(g_EntityList, *(uint32_t*)(targetPawn + g_Offsets.m_hPlayerPawn));
                        if (targetCtrl && targetCtrl > 0x100000) {
                            int targetTeam = *(uint8_t*)(targetCtrl + 0x840);
                            int localTeam = GetLocalTeam();
                            if (localTeam && targetTeam && (settings::triggerbotTeamCheck ? targetTeam != localTeam : true)) {
                                if (settings::triggerbotKey == 0 || (GetAsyncKeyState(settings::triggerbotKey) & 0x8000)) {
                                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    __try {
        uintptr_t localPawn = GetLocalPawn();
        if (localPawn && localPawn > 0x100000) {
            if (settings::noFlash)
                *(float*)(localPawn + 0x13FC) = 0.f;
            else
                *(float*)(localPawn + 0x13FC) = 255.f;
        }

        if (settings::fovChanger) {
            for (int i = 1; i <= 64; i++) {
                uintptr_t ctrl = GetEntity(g_EntityList, i);
                if (!ctrl || ctrl < 0x100000) continue;
                *(int*)(ctrl + 0x78C) = (int)settings::fovValue;
            }
        } else {
            for (int i = 1; i <= 64; i++) {
                uintptr_t ctrl = GetEntity(g_EntityList, i);
                if (!ctrl || ctrl < 0x100000) continue;
                *(int*)(ctrl + 0x78C) = 90;
            }
        }

        if (settings::noFog) {
            for (int i = 1; i <= 512; i++) {
                uintptr_t ent = GetEntity(g_EntityList, i);
                if (!ent || ent < 0x100000) continue;
                __try { *(float*)(ent + 0x628) = 0.f; } __except(EXCEPTION_EXECUTE_HANDLER) {}
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    if (settings::watermark && !menuOpen) {
        auto* list = ImGui::GetBackgroundDrawList();
        float fps = ImGui::GetIO().Framerate;
        static float ping = 0.f;
        __try {
            HMODULE engine = GetModuleHandleA("engine2.dll");
            if (engine) {
                uintptr_t netClient = *(uintptr_t*)((uintptr_t)engine + 0x9090C0);
                if (netClient && netClient > 0x100000)
                    ping = *(float*)(netClient + 0x280) * 1000.f;
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {}

        int sw = GetSystemMetrics(SM_CXSCREEN);
        char name[] = "CockEngine";
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

    ImGui::Render();

    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    return oPresent(pSwapChain, SyncInterval, Flags);
}

// --- Anti-debug / Anti-screenshot ---
void InitProtection() {
    auto NtSetInformationThread = (NTSTATUS(NTAPI*)(HANDLE, ULONG, PVOID, ULONG))
        GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");
    if (NtSetInformationThread)
        NtSetInformationThread(GetCurrentThread(), 0x11, nullptr, 0);

    HWND gameWnd = nullptr;
    while (!gameWnd) {
        gameWnd = FindWindowA("SDL_app", nullptr);
        Sleep(50);
    }
    SetWindowDisplayAffinity(gameWnd, WDA_EXCLUDEFROMCAPTURE);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
    g_hModule = (HMODULE)lpReserved;

    InitProtection();

    HMODULE client = nullptr;
    while (!client) {
        client = GetModuleHandleA("client.dll");
        Sleep(50);
    }

    InitRuntime();

    bool init_hook = false;
    do
    {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
        {
            kiero::bind(8, (void**)&oPresent, hkPresent);
            init_hook = true;
        }
    } while (!init_hook);

    InitCreateMoveHook();

    return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hMod);
        CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        kiero::shutdown();
        break;
    }
    return TRUE;
}
