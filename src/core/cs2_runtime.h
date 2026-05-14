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

        // CreateInterface
        auto clientCI = (CreateInterfaceFn)GetProcAddress(client, "CreateInterface");
        auto engineCI = (CreateInterfaceFn)GetProcAddress(engine, "CreateInterface");
        if (!clientCI || !engineCI) return false;

    printf("[Runtime] Interfaces OK\n");

        // SchemaSystem
        HMODULE schemaMod = GetModuleHandleA("schemasystem.dll");
        if (schemaMod) {
            auto schemaCI = (CreateInterfaceFn)GetProcAddress(schemaMod, "CreateInterface");
            InitSchemaSystem(schemaCI);
        }

        // Schema offsets (via SchemaSystem, fallback to defaults)
        g_Offsets.m_pGameSceneNode       = ResolveSchemaOffset("C_BaseEntity", "m_pGameSceneNode", 0x330);
        g_Offsets.m_iHealth              = ResolveSchemaOffset("C_BaseEntity", "m_iHealth", 0x34C);
        g_Offsets.m_iTeamNum             = ResolveSchemaOffset("CCSPlayerController", "m_iTeamNum", 0x3EB);
        g_Offsets.m_hPlayerPawn          = ResolveSchemaOffset("CCSPlayerController", "m_hPlayerPawn", 0x90C);
        g_Offsets.m_sSanitizedPlayerName = ResolveSchemaOffset("CCSPlayerController", "m_sSanitizedPlayerName", 0x860);
        g_Offsets.m_vecAbsOrigin         = ResolveSchemaOffset("CGameSceneNode", "m_vecAbsOrigin", 0xC8);
        g_Offsets.m_fFlags                = ResolveSchemaOffset("C_BaseEntity", "m_fFlags", 0x3F8);
        g_Offsets.m_modelState            = ResolveSchemaOffset("C_BaseEntity", "m_modelState", 0x340);
        printf("[Schema] m_iTeamNum = 0x%X\n", g_Offsets.m_iTeamNum);

        // Static globals (RVA only)
        g_Offsets.gameEntitySystem      = *(uintptr_t*)((uintptr_t)client + RVA_ENTITYLIST);
        g_Offsets.entityList            = g_Offsets.gameEntitySystem;
        g_Offsets.localPlayerPawn       = *(uintptr_t*)((uintptr_t)client + RVA_LOCALPAWN);
        g_Offsets.viewAngles            = (uintptr_t)client + RVA_VIEWANGLES;
        g_Offsets.viewMatrix            = (uintptr_t)client + RVA_VIEWMATRIX;

        printf("[Runtime] Init complete\n");
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        printf("[Runtime] CRASH\n");
        return false;
    }
}
