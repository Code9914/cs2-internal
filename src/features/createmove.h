#pragma once
#include "../core/includes.h"
#include "../core/vector.h"
#include "../core/entity.h"
#include "../core/pattern_scan.h"
#include "../core/hook.h"
#include "aimbot.h"

using CreateMoveFn = __int64(__fastcall*)(DWORD* a1, __int64 a2, char a3, double a4, int a5, __int64 a6);
inline CreateMoveFn oCreateMove = nullptr;
inline HookCtx g_CMHook = {};

// Global button state values
#define BTN_PRESS   0x10001  // bit 0 (pressed) + bit 16 (held)
#define BTN_RELEASE 0x0

inline void ForceJump(bool pressed) {
    HMODULE hClient = GetModuleHandleA(X("client.dll"));
    if (!hClient) return;

    uintptr_t jumpAddr = (uintptr_t)hClient + g_Offsets.btn_jump;
    uint32_t* pBtn = (uint32_t*)jumpAddr;

    *pBtn = pressed ? BTN_PRESS : BTN_RELEASE;
}

inline float NormalizeAngle(float angle) {
    while (angle > 180.f) angle -= 360.f;
    while (angle < -180.f) angle += 360.f;
    return angle;
}

inline Vector3 CalcAngle(const Vector3& src, const Vector3& dst) {
    Vector3 delta = dst - src;
    float dist = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    if (dist < 0.001f) return Vector3();

    float pitch = -asinf(delta.z / dist) * (180.f / 3.14159265358979323846f);
    float yaw   = atan2f(delta.y, delta.x) * (180.f / 3.14159265358979323846f);
    return Vector3(pitch, yaw, 0.f);
}

inline __int64 __fastcall hkCreateMove(DWORD* a1, __int64 a2, char a3, double a4, int a5, __int64 a6) {
    __try {
    // === Bhop ===
    static bool bhopReady = true;
    if (settings::bhop) {
        uintptr_t localPawn = GetLocalPawn();
        if (localPawn && localPawn > 0x100000) {
            int flags = *(int*)(localPawn + g_Offsets.m_fFlags);
            bool onGround = (flags & 1) != 0;
            bool spaceHeld = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

            if (!onGround) {
                ForceJump(false);
                bhopReady = true;
            } else if (spaceHeld && bhopReady) {
                ForceJump(true);
                bhopReady = false;
            }
        }
    }

    // === Silent Aimbot: compute angles BEFORE oCreateMove ===
    Vector3 silentAngles;
    bool silentFound = false;
    if (settings::aimbotSilent && settings::aimbotEnabled) {
        if (settings::aimbotKey == 0 || (GetAsyncKeyState(settings::aimbotKey) & 0x8000)) {
            __try {
            uintptr_t localPawn = GetLocalPawn();
            if (localPawn && localPawn > 0x100000) {
                int localTeam = GetLocalTeam();
                if (localTeam) {
                    uintptr_t sceneNode = *(uintptr_t*)(localPawn + g_Offsets.m_pGameSceneNode);
                    if (sceneNode && sceneNode >= 0x100000) {
                        Vector3 localPos = *(Vector3*)(sceneNode + g_Offsets.m_vecAbsOrigin);
                        localPos.z += *(float*)(localPawn + g_Offsets.m_vecViewOffset + 0x20);

                        float bestFov = (float)settings::aimbotFov;
                        Vector3 bestAngle;

                        float curPitch = 0.f, curYaw = 0.f;
                        if (a1 && (uintptr_t)a1 > 0x100000) {
                            curPitch = *(float*)((uintptr_t)a1 + 0x10);
                            curYaw = *(float*)((uintptr_t)a1 + 0x14);
                        }

                        for (int i = 1; i <= 64; i++) {
                            if (!players[i].valid) continue;
                            if (!players[i].pawn || players[i].pawn < 0x100000) continue;
                            if (settings::aimbotTeamCheck && players[i].team == localTeam) continue;

                            int boneIndices[] = { 0, 1, 4 };
                            int boneIdx = boneIndices[settings::aimbotHitbox];
                            Vector3 aimPos = GetBonePos(players[i].pawn, boneIdx);
                            if (aimPos.x == 0.f && aimPos.y == 0.f && aimPos.z == 0.f) continue;

                            Vector3 angle = CalcAngle(localPos, aimPos);
                            float dp = NormalizeAngle(angle.x - curPitch);
                            float dy = NormalizeAngle(angle.y - curYaw);
                            float fov = sqrtf(dp * dp + dy * dy);
                            if (fov < bestFov) {
                                bestFov = fov;
                                bestAngle = angle;
                                silentFound = true;
                            }
                        }
                        if (silentFound) silentAngles = bestAngle;
                    }
                }
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {}
        }
    }

    // Write silent aim angles to input state before CreateMove copies them
    if (silentFound && a1 && (uintptr_t)a1 > 0x100000) {
        __try {
            *(float*)((uintptr_t)a1 + 0x10) = silentAngles.x;
            *(float*)((uintptr_t)a1 + 0x14) = silentAngles.y;
        } __except(EXCEPTION_EXECUTE_HANDLER) {}
    }

    __int64 result = oCreateMove(a1, a2, a3, a4, a5, a6);

    // === Normal Aimbot: write to viewAngles AFTER oCreateMove ===
    if (!settings::aimbotSilent) {
        if (!settings::aimbotEnabled) return result;

        if (settings::aimbotKey != 0 && !(GetAsyncKeyState(settings::aimbotKey) & 0x8000))
            return result;

        if (!g_Offsets.viewAngles || g_Offsets.viewAngles < 0x100000) return result;

        uintptr_t localPawn = GetLocalPawn();
        if (!localPawn || localPawn < 0x100000) return result;

        int localTeam = GetLocalTeam();
        if (!localTeam) return result;

        uintptr_t sceneNode = *(uintptr_t*)(localPawn + g_Offsets.m_pGameSceneNode);
        if (!sceneNode || sceneNode < 0x100000) return result;

        Vector3 localPos = *(Vector3*)(sceneNode + g_Offsets.m_vecAbsOrigin);
        localPos.z += *(float*)(localPawn + g_Offsets.m_vecViewOffset + 0x20);

        float bestFov = (float)settings::aimbotFov;
        Vector3 bestAngle;
        bool found = false;

        for (int i = 1; i <= 64; i++) {
            if (!players[i].valid) continue;
            if (!players[i].pawn || players[i].pawn < 0x100000) continue;
            if (settings::aimbotTeamCheck && players[i].team == localTeam) continue;

            int boneIndices[] = { 0, 1, 4 };
            int boneIdx = boneIndices[settings::aimbotHitbox];
            Vector3 aimPos = GetBonePos(players[i].pawn, boneIdx);
            if (aimPos.x == 0.f && aimPos.y == 0.f && aimPos.z == 0.f) continue;

            Vector3 angle = CalcAngle(localPos, aimPos);

            float curPitch = *(float*)(g_Offsets.viewAngles + 0x0);
            float curYaw   = *(float*)(g_Offsets.viewAngles + 0x4);

            float dp = NormalizeAngle(angle.x - curPitch);
            float dy = NormalizeAngle(angle.y - curYaw);

            float fov = sqrtf(dp * dp + dy * dy);
            if (fov < bestFov) {
                bestFov = fov;
                bestAngle = angle;
                found = true;
            }
        }

        if (found) {
            float curPitch = *(float*)(g_Offsets.viewAngles + 0x0);
            float curYaw   = *(float*)(g_Offsets.viewAngles + 0x4);

            if (settings::aimbotSmoothEnabled && settings::aimbotSmooth > 1.f) {
                float dp = NormalizeAngle(bestAngle.x - curPitch);
                float dy = NormalizeAngle(bestAngle.y - curYaw);

                dp /= settings::aimbotSmooth;
                dy /= settings::aimbotSmooth;

                float newPitch = curPitch + dp;
                float newYaw = curYaw + dy;
                if (newPitch < -89.f) newPitch = -89.f;
                if (newPitch > 89.f) newPitch = 89.f;

                *(float*)(g_Offsets.viewAngles + 0x0) = newPitch;
                *(float*)(g_Offsets.viewAngles + 0x4) = newYaw;
            } else {
                float p = bestAngle.x;
                float y = bestAngle.y;
                if (p < -89.f) p = -89.f;
                if (p > 89.f) p = 89.f;

                *(float*)(g_Offsets.viewAngles + 0x0) = p;
                *(float*)(g_Offsets.viewAngles + 0x4) = y;
            }
        }
    }

    return result;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return oCreateMove(a1, a2, a3, a4, a5, a6);
    }
}

inline bool InitCreateMoveHook(uintptr_t* outAddr = nullptr) {
    HMODULE client = GetModuleHandleA(X("client.dll"));
    if (!client) return false;

    uintptr_t base = (uintptr_t)client;

    const char* pattern = X("48 89 5C 24 ?? 55 57 41 56 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 8B 01 48 8B F9");
    auto sig = Signature::FromString(pattern);
    uintptr_t addr = PatternScan::Find(sig, client);

    if (!addr) {
        addr = base + 0xC57F70;
    }

    if (outAddr) *outAddr = addr;

    g_CMHook.target = (void*)addr;
    g_CMHook.detour = (void*)hkCreateMove;

    bool ok = HookInstall(&g_CMHook);
    if (!ok) return false;

    oCreateMove = (CreateMoveFn)g_CMHook.original;
    return true;
}
