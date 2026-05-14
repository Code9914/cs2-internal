#pragma once
#include "includes.h"
#include "vector.h"
#include "entity.h"
#include "aimbot.h"
#include "kiero/minhook/include/MinHook.h"

using CreateMoveFn = void(__fastcall*)(__int64 a1, __int64 a2);
inline CreateMoveFn oCreateMove = nullptr;

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

inline void __fastcall hkCreateMove(__int64 a1, __int64 a2) {
    oCreateMove(a1, a2);

    if (!settings::aimbotEnabled) return;

    if (settings::aimbotKey != 0 && !(GetAsyncKeyState(settings::aimbotKey) & 0x8000))
        return;

    uintptr_t localPawn = GetLocalPawn();
    if (!localPawn) return;

    int localTeam = GetLocalTeam();
    if (!localTeam) return;

    uintptr_t sceneNode = *(uintptr_t*)(localPawn + g_Offsets.m_pGameSceneNode);
    if (!sceneNode) return;
    Vector3 localPos = *(Vector3*)(sceneNode + g_Offsets.m_vecAbsOrigin);
    localPos.z += *(float*)(localPawn + g_Offsets.m_vecViewOffset + 0x20);

    float bestFov = settings::aimbotFov;
    Vector3 bestAngle;
    bool found = false;

    for (int i = 1; i <= 64; i++) {
        if (!players[i].valid) continue;
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
        if (settings::aimbotSmoothEnabled && settings::aimbotSmooth > 1.f) {
            float curPitch = *(float*)(g_Offsets.viewAngles + 0x0);
            float curYaw   = *(float*)(g_Offsets.viewAngles + 0x4);

            float dp = NormalizeAngle(bestAngle.x - curPitch);
            float dy = NormalizeAngle(bestAngle.y - curYaw);

            dp /= settings::aimbotSmooth;
            dy /= settings::aimbotSmooth;

            *(float*)(g_Offsets.viewAngles + 0x0) = curPitch + dp;
            *(float*)(g_Offsets.viewAngles + 0x4) = curYaw + dy;
        } else {
            *(float*)(g_Offsets.viewAngles + 0x0) = bestAngle.x;
            *(float*)(g_Offsets.viewAngles + 0x4) = bestAngle.y;
        }
    }
}

inline bool InitCreateMoveHook() {
    HMODULE client = GetModuleHandleA("client.dll");
    if (!client) {
        printf("[CreateMove] FAIL: client.dll not found\n");
        return false;
    }

    uintptr_t base = (uintptr_t)client;
    uintptr_t funcAbs = base + 0x85ddb0;

    printf("[CreateMove] base=0x%llX | funcAbs=0x%llX | viewAngles=0x%llX\n", base, funcAbs, g_Offsets.viewAngles);

    __try {
        MH_STATUS status = MH_CreateHook((LPVOID)funcAbs, hkCreateMove, (LPVOID*)&oCreateMove);
        if (status != MH_OK) {
            printf("[CreateMove] MH_CreateHook failed: %d\n", status);
            return false;
        }

        status = MH_EnableHook((LPVOID)funcAbs);
        if (status != MH_OK) {
            printf("[CreateMove] MH_EnableHook failed: %d\n", status);
            return false;
        }

        printf("[CreateMove] Hook installed (orig=0x%llX)\n", (uintptr_t)oCreateMove);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("[CreateMove] FAIL: exception\n");
        return false;
    }
}
