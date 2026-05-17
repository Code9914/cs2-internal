#pragma once
#include "includes.h"
#include "vector.h"

inline uintptr_t g_EntityList = 0;

inline uintptr_t GetEntity(uintptr_t entityList, uint32_t idx) {
    __try {
        uintptr_t chunk = *(uintptr_t*)(entityList + 8 * ((idx & 0x7FFF) >> 9));
        if (chunk < 0x100000) return 0;
        uintptr_t entity = *(uintptr_t*)(chunk + 112 * (idx & 0x1FF));
        return (entity > 0x100000) ? entity : 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

inline Vector3 GetBonePos(uintptr_t pawn, int boneId) {
    __try {
        uintptr_t boneMatrix = *(uintptr_t*)(pawn + 0x1D80);
        if (!boneMatrix || boneMatrix < 0x100000) return Vector3();

        float* bone = (float*)(boneMatrix + boneId * 0x20);
        return Vector3(bone[0], bone[1], bone[2]);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return Vector3(); }
}

inline uintptr_t GetLocalPawn() {
    __try {
        HMODULE client = GetModuleHandleA(X("client.dll"));
        if (!client) return 0;
        uintptr_t pawn = *(uintptr_t*)((uintptr_t)client + RVA_LOCALPAWN);
        return (pawn > 0x100000) ? pawn : 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

inline int GetLocalTeam() {
    __try {
        uintptr_t lp = GetLocalPawn();
        if (!lp) return 0;
        if (!g_EntityList || g_EntityList < 0x100000) return 0;
        for (int i = 1; i <= 64; i++) {
            uintptr_t ctrl = GetEntity(g_EntityList, i);
            if (!ctrl || ctrl < 0x100000) continue;
            uint32_t ph = *(uint32_t*)(ctrl + g_Offsets.m_hPlayerPawn);
            if (!ph) continue;
            uintptr_t pawn = GetEntity(g_EntityList, ph);
            if (pawn && pawn == lp)
                return *(uint8_t*)(ctrl + g_Offsets.m_iTeamNumCtrl);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    return 0;
}

struct PlayerData {
    uintptr_t controller;
    uintptr_t pawn;
    Vector3 pos;
    Vector3 headPos;
    int health;
    int team;
    char name[128];
    bool valid;
};

inline PlayerData players[65];

inline void UpdateEntities() {
    __try {
    ZeroMemory(players, sizeof(players));

    g_EntityList = g_Offsets.entityList;
    if (g_Offsets.gameEntitySystem) {
        g_EntityList = g_Offsets.gameEntitySystem + 0x10;
    }

    if (!g_EntityList || g_EntityList < 0x100000) return;

    uintptr_t localPawn = GetLocalPawn();

    for (int i = 1; i <= 64; i++) {
        __try {
            uintptr_t controller = GetEntity(g_EntityList, i);
            if (!controller || controller < 0x100000) continue;

            uint32_t pawnHandle = *(uint32_t*)(controller + g_Offsets.m_hPlayerPawn);
            if (!pawnHandle) continue;

            uintptr_t pawn = GetEntity(g_EntityList, pawnHandle);
            if (!pawn || pawn < 0x100000) continue;

            if (localPawn > 0x100000 && pawn == localPawn)
                continue;

            int health = *(int*)(pawn + g_Offsets.m_iHealth);
            if (health <= 0 || health > 100) continue;

            int team = *(uint8_t*)(controller + g_Offsets.m_iTeamNumCtrl);
            if (team != 2 && team != 3) continue;

            uintptr_t sceneNode = *(uintptr_t*)(pawn + g_Offsets.m_pGameSceneNode);
            if (!sceneNode || sceneNode < 0x100000) continue;

            Vector3 pos = *(Vector3*)(sceneNode + g_Offsets.m_vecAbsOrigin);
            Vector3 headPos = GetBonePos(pawn, 0);

            players[i].controller = controller;
            players[i].pawn = pawn;
            players[i].pos = pos;
            players[i].headPos = headPos;
            players[i].health = health;
            players[i].team = team;
            players[i].name[0] = '\0';

            // Try m_sSanitizedPlayerName (CUtlString, pointer at offset 0)
            uintptr_t nameAddr = controller + g_Offsets.m_sSanitizedPlayerName;
            if (nameAddr > 0x100000) {
                const char* namePtr = *(const char**)(nameAddr);
                if (namePtr && (uintptr_t)namePtr > 0x100000) {
                    __try {
                        strcpy_s(players[i].name, namePtr);
                    } __except(EXCEPTION_EXECUTE_HANDLER) {}
                }
            }

            // Fallback: m_iszPlayerName (char[128] inline)
            if (!players[i].name[0]) {
                const char* inlineName = (const char*)(controller + g_Offsets.m_iszPlayerName);
                if (inlineName && (uintptr_t)inlineName > 0x100000 && inlineName[0]) {
                    __try {
                        strcpy_s(players[i].name, inlineName);
                    } __except(EXCEPTION_EXECUTE_HANDLER) {}
                }
            }

            players[i].valid = true;

        } __except(EXCEPTION_EXECUTE_HANDLER) {}
    }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
}
