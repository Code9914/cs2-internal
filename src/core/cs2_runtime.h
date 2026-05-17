#pragma once
#include "includes.h"
#include "entity.h"
#include "vector.h"
#include "schema_system.h"
#include "pattern_scan.h"

// ============================================================================
// Main init
// RVA defines in game_offsets.h (available to all translation units)
// ============================================================================

// Pattern: 85 D2 75 3D 48 63 81 ? ? ? ?
// CCSGOInput::SetViewAngle(this, nSlot, pAngle)
// Params: RCX=this, RDX=slot, R8=QAngle*(pitch,yaw,roll as 3 floats)
using SetViewAngleFn = void(__fastcall*)(uintptr_t, int, const float*);
inline SetViewAngleFn g_SetViewAngle = nullptr;

inline void InitSetViewAngle(HMODULE client) {
    // Manual pattern scan for: 85 D2 75 3D 48 63 81 ? ? ? ?
    uint8_t pattern[] = { 0x85, 0xD2, 0x75, 0x3D, 0x48, 0x63, 0x81 };
    uint8_t mask[]    = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    auto dos = (const IMAGE_DOS_HEADER*)client;
    auto nt  = (const IMAGE_NT_HEADERS*)((uintptr_t)client + dos->e_lfanew);
    uintptr_t base = (uintptr_t)client;
    auto sections = IMAGE_FIRST_SECTION(nt);

    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        auto& sec = sections[i];
        if (!(sec.Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        uintptr_t secStart = base + sec.VirtualAddress;
        size_t secSize = sec.Misc.VirtualSize;
        if (!secSize) secSize = sec.SizeOfRawData;
        if (!secSize) continue;

        for (size_t j = 0; j <= secSize - sizeof(pattern); ++j) {
            bool match = true;
            for (size_t k = 0; k < sizeof(pattern); ++k) {
                if (mask[k] == 0xFF && ((uint8_t*)(secStart + j))[k] != pattern[k]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                g_SetViewAngle = (SetViewAngleFn)(secStart + j);
                return;
            }
        }
    }
}

inline bool InitRuntime() {
    __try {
        HMODULE client = GetModuleHandleA("client.dll");
        HMODULE engine = GetModuleHandleA("engine2.dll");
        if (!client || !engine) return false;

        auto clientCI = (CreateInterfaceFn)GetProcAddress(client, "CreateInterface");
        auto engineCI = (CreateInterfaceFn)GetProcAddress(engine, "CreateInterface");
        if (!clientCI || !engineCI) return false;

        extern void* g_EngineClient;
        g_EngineClient = engineCI("Source2EngineToClient001", nullptr);

        HMODULE schemaMod = GetModuleHandleA("schemasystem.dll");
        if (schemaMod) {
            auto schemaCI = (CreateInterfaceFn)GetProcAddress(schemaMod, "CreateInterface");
            InitSchemaSystem(schemaCI);
        }

        g_Offsets.gameEntitySystem      = *(uintptr_t*)((uintptr_t)client + RVA_ENTITYLIST);
        g_Offsets.entityList            = g_Offsets.gameEntitySystem;
        g_Offsets.localPlayerPawn       = *(uintptr_t*)((uintptr_t)client + RVA_LOCALPAWN);
        g_Offsets.viewAngles            = (uintptr_t)client + RVA_VIEWANGLES;
        g_Offsets.viewMatrix            = (uintptr_t)client + RVA_VIEWMATRIX;

        InitSetViewAngle(client);

        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
