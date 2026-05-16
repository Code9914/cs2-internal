#pragma once
#include <cstdio>
#include <cstdint>

// RVA from cs2-dumper (runtime pointers in client.dll)
// Updated: 2026-05-15
#define RVA_ENTITYLIST     0x24D4E80
#define RVA_LOCALPAWN      0x205A700
#define RVA_VIEWANGLES     0x23444F8
#define RVA_VIEWMATRIX     0x2334850

// All values resolved at runtime via:
//   - CreateInterface (interfaces)
//   - SchemaSystem (class field offsets)
//   - Pattern scanning (globals)

struct GameOffsets {
    // Entity system
    uintptr_t entityList            = 0;
    uintptr_t localPlayerPawn       = 0;
    uintptr_t gameEntitySystem      = 0;

    // View
    uintptr_t viewMatrix            = 0;
    uintptr_t viewAngles            = 0;

    // Schema offsets (resolved by SchemaSystem at runtime)
    int32_t m_pGameSceneNode        = 0x330;
    int32_t m_iHealth               = 0x34C;
    int32_t m_iTeamNum              = 0x3EB;
    int32_t m_hPlayerPawn           = 0x90C;
    int32_t m_sSanitizedPlayerName  = 0x860;
    int32_t m_iszPlayerName         = 0x6F4; // char[128] in CBasePlayerController
    int32_t m_vecAbsOrigin          = 0xC8;
    int32_t m_fFlags                = 0x3F8;
    int32_t m_vecViewOffset         = 0xE70;
    int32_t m_modelState            = 0x1D0;
    int32_t m_pBoneArray            = 0x80;
    
    // Movement
    int32_t m_pMovementServices     = 0x1220;
    int32_t m_nButtons              = 0x50; // Relative to m_pMovementServices
    int32_t m_nQueuedButtonDownMask = 0x70; // Relative to m_pMovementServices
    int32_t m_nQueuedButtonChangeMask = 0x78; // Relative to m_pMovementServices

    // Global button offsets (relative to client.dll base)
    int32_t btn_jump                = 0x2053EA0;
    int32_t btn_forward             = 0x2053BD0;
    int32_t btn_back                = 0x2053C60;
    int32_t btn_left                = 0x2053CF0;
    int32_t btn_right               = 0x2053D80;
    int32_t btn_duck                = 0x2053F30;
    int32_t btn_sprint              = 0x2053870;
};

inline GameOffsets g_Offsets;
