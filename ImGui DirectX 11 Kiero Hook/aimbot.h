#pragma once
#include "includes.h"

namespace settings {
    inline bool   aimbotEnabled   = false;
    inline int    aimbotKey       = VK_LBUTTON;
    inline float  aimbotFov       = 8.f;
    inline float  aimbotSmooth    = 6.f;
    inline int    aimbotHitbox    = 0;
    inline bool   aimbotTeamCheck = true;
    inline bool   aimbotAutoFire  = false;
    inline bool   aimbotShowFov   = false;
    inline bool   aimbotKeyWait   = false;
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
