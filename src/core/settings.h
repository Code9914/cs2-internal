#pragma once
#include <Windows.h>

namespace settings {
    // --- ESP ---
    inline bool   espEnabled     = true;
    inline bool   espTeamCheck   = true;
    inline bool   box            = true;
    inline bool   boxFilled      = false;
    inline bool   boxCorner      = false;
    inline float  boxThickness   = 2.f;
    inline float  boxColor[4]    = { 1.f, 0.55f, 0.f, 1.f };
    inline float  boxFilledColor[4] = { 0.f, 0.f, 0.f, 0.3f };
    inline bool   health         = true;
    inline float  healthColor[4] = { 0.f, 1.f, 0.f, 1.f };
    inline bool   name           = true;
    inline float  nameColor[4]   = { 1.f, 1.f, 1.f, 1.f };
    inline bool   distance       = false;
    inline float  distColor[4]   = { 1.f, 1.f, 1.f, 1.f };

    // --- Aimbot ---
    inline bool   aimbotEnabled      = false;
    inline int    aimbotKey          = VK_LBUTTON;
    inline bool   aimbotKeyWait      = false;
    inline int    aimbotHitbox       = 0;
    inline float  aimbotFov          = 8.f;
    inline bool   aimbotShowFov      = false;
    inline bool   aimbotSmoothEnabled = false;
    inline float  aimbotSmooth       = 6.f;
    inline bool   aimbotTeamCheck    = true;

    // --- Triggerbot ---
    inline bool   triggerbotEnabled   = false;
    inline int    triggerbotKey       = VK_MENU;
    inline bool   triggerbotKeyWait   = false;
    inline bool   triggerbotTeamCheck = true;

    // --- Visual ---
    inline bool   noFlash    = false;
    inline bool   fovChanger = false;
    inline float  fovValue   = 90.f;
    inline bool   noFog      = false;

    // --- Misc ---
    inline bool   watermark  = true;
}
