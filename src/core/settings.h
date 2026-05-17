#pragma once
#include <Windows.h>

namespace settings {
    // --- ESP ---
    inline bool   espEnabled     = false;
    inline bool   espTeamCheck   = true;
    inline bool   box            = false;
    inline bool   boxFilled      = false;
    inline bool   boxCorner      = false;
    inline int    boxThickness   = 2;
    inline float  boxColor[4]    = { 1.f, 0.55f, 0.f, 1.f };
    inline float  boxFilledColor[4] = { 0.f, 0.f, 0.f, 0.3f };
    inline bool   health         = false;
    inline float  healthColor[4] = { 0.f, 1.f, 0.f, 1.f };
    inline bool   name           = false;
    inline float  nameColor[4]   = { 1.f, 1.f, 1.f, 1.f };
    inline bool   distance       = false;
    inline float  distColor[4]   = { 1.f, 1.f, 1.f, 1.f };

    // --- Aimbot ---
    inline bool   aimbotEnabled      = false;
    inline bool   aimbotSilent       = false;
    inline int    aimbotKey          = VK_LBUTTON;
    inline bool   aimbotKeyWait      = false;
    inline int    aimbotHitbox       = 0;
    inline int    aimbotFov          = 8;
    inline bool   aimbotShowFov      = false;
    inline bool   aimbotSmoothEnabled = false;
    inline int    aimbotSmooth       = 6;
    inline bool   aimbotTeamCheck    = true;

    // --- Triggerbot ---
    inline bool   triggerbotEnabled   = false;
    inline int    triggerbotKey       = VK_MENU;
    inline bool   triggerbotKeyWait   = false;
    inline bool   triggerbotTeamCheck = true;

    // --- Visual ---
    inline bool   noFlash    = false;
    inline bool   fovChanger = false;
    inline int    fovValue   = 90;
    inline bool   noFog      = false;

    // --- Misc ---
    inline bool   bhop       = false;
    inline bool   watermark  = false;
}
