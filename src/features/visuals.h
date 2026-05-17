#pragma once
#include "../core/includes.h"
#include "../core/entity.h"

inline void ApplyVisuals() {
    __try {
        uintptr_t localPawn = GetLocalPawn();
        if (localPawn && localPawn > 0x100000) {
            if (settings::noFlash)
                *(float*)(localPawn + 0x13FC) = 0.f;
            else
                *(float*)(localPawn + 0x13FC) = 255.f;
        }

        if (settings::fovChanger) {
            for (int i = 1; i <= 64; i++) {
                uintptr_t ctrl = GetEntity(g_EntityList, i);
                if (!ctrl || ctrl < 0x100000) continue;
                *(int*)(ctrl + 0x78C) = (int)settings::fovValue;
            }
        } else {
            for (int i = 1; i <= 64; i++) {
                uintptr_t ctrl = GetEntity(g_EntityList, i);
                if (!ctrl || ctrl < 0x100000) continue;
                *(int*)(ctrl + 0x78C) = 90;
            }
        }

        if (settings::noFog) {
            for (int i = 1; i <= 512; i++) {
                uintptr_t ent = GetEntity(g_EntityList, i);
                if (!ent || ent < 0x100000) continue;
                __try { *(float*)(ent + 0x628) = 0.f; } __except(EXCEPTION_EXECUTE_HANDLER) {}
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
}
