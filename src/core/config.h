#pragma once
#include "../core/includes.h"

#pragma warning(disable: 4474 4477 4996)

inline char g_ConfigPath[MAX_PATH] = {};

inline void SaveConfig() {
    FILE* f = nullptr;
    if (fopen_s(&f, g_ConfigPath, "w") || !f) return;

    fprintf(f, "%d %d %d %d %d %d %d %d %d %d "
        "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f "
        "%d %d %d %d %d %d %d %d %d %d %d %d "
        "%d %d %d %d %d %d",
        settings::box, settings::boxFilled, settings::boxCorner,
        settings::health, settings::name, settings::distance,
        settings::espEnabled, settings::espTeamCheck, settings::watermark,
        settings::boxThickness,
        settings::boxColor[0], settings::boxColor[1], settings::boxColor[2], settings::boxColor[3],
        settings::boxFilledColor[0], settings::boxFilledColor[1], settings::boxFilledColor[2], settings::boxFilledColor[3],
        settings::healthColor[0], settings::healthColor[1], settings::healthColor[2], settings::healthColor[3],
        settings::nameColor[0], settings::nameColor[1], settings::nameColor[2], settings::nameColor[3],
        settings::distColor[0], settings::distColor[1], settings::distColor[2], settings::distColor[3],
        settings::aimbotEnabled, settings::aimbotTeamCheck, settings::aimbotShowFov,
        settings::triggerbotEnabled, settings::triggerbotTeamCheck,
        settings::aimbotFov, settings::aimbotSmooth, settings::aimbotSmoothEnabled,
        settings::aimbotKey, settings::triggerbotKey, settings::aimbotHitbox, settings::fovValue,
        settings::noFlash, settings::fovChanger, settings::noFog,
        settings::bhop, settings::aimbotKeyWait, settings::triggerbotKeyWait);
    fclose(f);
}

inline void LoadConfig() {
    FILE* f = nullptr;
    if (fopen_s(&f, g_ConfigPath, "r") || !f) return;

    // Initialize with defaults
    int b = 0, bf = 0, bc = 0, hp = 0, nm = 0, dist = 0, esp = 0, et = 1, wm = 0;
    int bt = 2;
    float bcol[4] = {1, 0.55f, 0, 1}, bfill[4] = {0, 0, 0, 0.3f};
    float hcol[4] = {0, 1, 0, 1}, ncol[4] = {1, 1, 1, 1}, dcol[4] = {1, 1, 1, 1};
    int ae = 0, tc = 1, sf = 0, te = 0, ttk = 1;
    int fov = 8, sm = 6;
    int se = 0, ak = VK_LBUTTON, ttk2 = VK_MENU, ah = 0;
    int fv = 90;
    int nf = 0, fc = 0, nfg = 0, bh = 0, akw = 0, tkw = 0;

    fscanf(f, "%d %d %d %d %d %d %d %d %d %d "
        "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f "
        "%d %d %d %d %d %d %d %d %d %d %d %d "
        "%d %d %d %d %d %d",
        &b, &bf, &bc, &hp, &nm, &dist, &esp, &et, &wm, &bt,
        &bcol[0], &bcol[1], &bcol[2], &bcol[3],
        &bfill[0], &bfill[1], &bfill[2], &bfill[3],
        &hcol[0], &hcol[1], &hcol[2], &hcol[3],
        &ncol[0], &ncol[1], &ncol[2], &ncol[3],
        &dcol[0], &dcol[1], &dcol[2], &dcol[3],
        &ae, &tc, &sf, &te, &ttk,
        &fov, &sm, &se, &ak, &ttk2, &ah, &fv,
        &nf, &fc, &nfg, &bh, &akw, &tkw);
    fclose(f);

    settings::box = b != 0; settings::boxFilled = bf != 0; settings::boxCorner = bc != 0;
    settings::health = hp != 0; settings::name = nm != 0;
    settings::distance = dist != 0; settings::espEnabled = esp != 0;
    settings::espTeamCheck = et != 0;
    settings::watermark = wm != 0;
    settings::boxThickness = bt;
    memcpy(settings::boxColor, bcol, sizeof(bcol));
    memcpy(settings::boxFilledColor, bfill, sizeof(bfill));
    memcpy(settings::healthColor, hcol, sizeof(hcol));
    memcpy(settings::nameColor, ncol, sizeof(ncol));
    memcpy(settings::distColor, dcol, sizeof(dcol));
    settings::aimbotEnabled = ae != 0; settings::aimbotTeamCheck = tc != 0;
    settings::aimbotShowFov = sf != 0;
    settings::triggerbotEnabled = te != 0; settings::triggerbotTeamCheck = ttk != 0;
    settings::aimbotFov = fov; settings::aimbotSmooth = sm; settings::aimbotSmoothEnabled = se != 0;
    settings::aimbotKey = ak; settings::triggerbotKey = ttk2;
    settings::aimbotHitbox = ah; settings::fovValue = fv;
    settings::noFlash = nf != 0; settings::fovChanger = fc != 0; settings::noFog = nfg != 0;
    settings::bhop = bh != 0; settings::aimbotKeyWait = akw != 0; settings::triggerbotKeyWait = tkw != 0;
}

inline void InitConfigPath(HMODULE hMod) {
    char appData[MAX_PATH];
    GetEnvironmentVariableA("APPDATA", appData, MAX_PATH);
    strcat_s(appData, "\\CockEngine");
    CreateDirectoryA(appData, nullptr);
    strcpy_s(g_ConfigPath, appData);
    strcat_s(g_ConfigPath, "\\config.cfg");
}
