#pragma once
#include "../core/includes.h"

inline char g_ConfigPath[MAX_PATH] = {};

inline void SaveConfig() {
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

inline void LoadConfig() {
    FILE* f = nullptr;
    if (fopen_s(&f, g_ConfigPath, "rb") || !f) return;

    int b = 1, bf = 0, bc = 0, hp = 1, nm = 1, dist = 0, esp = 1, et = 1, wm = 1;
    int ae = 0, tc = 1, sf = 0, te = 0, tk = 0, se = 0;
    int ak = VK_LBUTTON, tk2 = VK_MENU, ah = 0, nf = 0, fc = 0, nfg = 0;
    float bt = 2.f, fov = 8.f, sm = 6.f, fv = 90.f;
    float bcol[4] = {1, 0.55f, 0, 1}, bfill[4] = {0, 0, 0, 0.3f};
    float hcol[4] = {0, 1, 0, 1}, ncol[4] = {1, 1, 1, 1}, dcol[4] = {1, 1, 1, 1};

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

    settings::box = b != 0; settings::boxFilled = bf != 0; settings::boxCorner = bc != 0;
    settings::health = hp != 0; settings::name = nm != 0;
    settings::distance = dist != 0; settings::espEnabled = esp != 0;
    settings::espTeamCheck = et != 0;
    settings::watermark = wm != 0;
    settings::aimbotEnabled = ae != 0; settings::aimbotTeamCheck = tc != 0;
    settings::aimbotShowFov = sf != 0;
    settings::triggerbotEnabled = te != 0; settings::triggerbotTeamCheck = tk != 0;
    settings::aimbotFov = fov; settings::aimbotSmooth = sm; settings::aimbotSmoothEnabled = se != 0;
    settings::aimbotKey = ak; settings::triggerbotKey = tk2;
    settings::aimbotHitbox = ah; settings::fovValue = fv;
    settings::noFlash = nf != 0; settings::fovChanger = fc != 0; settings::noFog = nfg != 0;
    settings::boxThickness = bt;
    memcpy(settings::boxColor, bcol, 16);
    memcpy(settings::boxFilledColor, bfill, 16);
    memcpy(settings::healthColor, hcol, 16);
    memcpy(settings::nameColor, ncol, 16);
    memcpy(settings::distColor, dcol, 16);
}

inline void InitConfigPath(HMODULE hMod) {
    GetModuleFileNameA(hMod, g_ConfigPath, MAX_PATH);
    char* dot = strrchr(g_ConfigPath, '.');
    if (dot) strcpy_s(dot, 5, ".cfg");
}
