#pragma once

// Obfuscate MinHook public API names
#define MH_Initialize           hk_Init
#define MH_Uninitialize         hk_Uninit
#define MH_CreateHook           hk_CreateHook
#define MH_CreateHookApi        hk_CreateHookApi
#define MH_RemoveHook           hk_RemoveHook
#define MH_EnableHook           hk_EnableHook
#define MH_DisableHook          hk_DisableHook
#define MH_QueueEnableHook      hk_QueueEnableHook
#define MH_QueueDisableHook     hk_QueueDisableHook
#define MH_ApplyQueued          hk_ApplyQueued
#define MH_EnumerateHooks       hk_EnumHooks
#define MH_SetThreadTimeout     hk_SetTimeout
#define MH_StatusToString       hk_StatusToStr
