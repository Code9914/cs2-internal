#pragma once
#include "../core/includes.h"
#include "../core/entity.h"

inline void RunTriggerbot() {
    if (!settings::triggerbotEnabled) return;

    uintptr_t localPawn = GetLocalPawn();
    if (!localPawn || localPawn < 0x100000) return;

    int idEntIndex = *(int*)(localPawn + 0x344C);
    if (idEntIndex <= 0 || idEntIndex >= 64) return;

    uintptr_t targetPawn = GetEntity(g_EntityList, idEntIndex);
    if (!targetPawn || targetPawn < 0x100000) return;

    uintptr_t targetCtrl = GetEntity(g_EntityList, *(uint32_t*)(targetPawn + g_Offsets.m_hPlayerPawn));
    if (!targetCtrl || targetCtrl < 0x100000) return;

    int targetTeam = *(uint8_t*)(targetCtrl + 0x840);
    int localTeam = GetLocalTeam();
    if (!localTeam || !targetTeam) return;
    if (settings::triggerbotTeamCheck && targetTeam == localTeam) return;

    if (settings::triggerbotKey == 0 || (GetAsyncKeyState(settings::triggerbotKey) & 0x8000)) {
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    }
}
