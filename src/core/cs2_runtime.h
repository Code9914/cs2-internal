#pragma once
#include "includes.h"
#include "entity.h"
#include "schema_system.h"

// ============================================================================
// Main init
// RVA defines in game_offsets.h (available to all translation units)
// ============================================================================

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

        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
