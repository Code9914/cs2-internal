#pragma once
#include "includes.h"

namespace settings {
    inline bool   aimbotEnabled   = false;
    inline int    aimbotKey       = VK_LBUTTON;
    inline float  aimbotFov       = 8.f;
    inline float  aimbotSmooth    = 6.f;
    inline bool   aimbotSmoothEnabled = false;
    inline int    aimbotHitbox    = 0;
    inline bool   aimbotTeamCheck = true;
    inline bool   aimbotShowFov   = false;
    inline bool   aimbotKeyWait   = false;

    // --- Triggerbot ---
    inline bool   triggerbotEnabled = false;
    inline int    triggerbotKey     = VK_MENU;
    inline bool   triggerbotTeamCheck = true;
    inline bool   triggerbotKeyWait = false;
}

inline const char* AimbotKeyName(int key) {
    switch (key) {
        case VK_LBUTTON: return "Mouse1";
        case VK_RBUTTON: return "Mouse2";
        case VK_MBUTTON: return "Mouse3";
        case VK_XBUTTON1: return "Mouse4";
        case VK_XBUTTON2: return "Mouse5";
        case VK_CONTROL: return "Ctrl";
        case VK_SHIFT: return "Shift";
        case VK_MENU: return "Alt";
        case VK_CAPITAL: return "Caps";
        case VK_TAB: return "Tab";
        case VK_SPACE: return "Space";
        case VK_RETURN: return "Enter";
        default:
            if (key >= 'A' && key <= 'Z') { static char b[2]; b[0] = (char)key; return b; }
            if (key >= '0' && key <= '9') { static char b[2]; b[0] = (char)key; return b; }
            return "?";
    }
}

inline void DrawFOV() {
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    float radius = (settings::aimbotFov / 180.f) * screenW;
    ImVec2 center((float)screenW / 2, (float)screenH / 2);
    draw->AddCircle(center, radius, IM_COL32(255, 255, 255, 150), 120, 1.5f);
}
