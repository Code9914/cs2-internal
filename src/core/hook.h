#pragma once
#include <Windows.h>
#include <cstdint>

struct HookCtx {
    void* target;
    void* detour;
    void* original;
    uint8_t origBytes[16];
    size_t origLen;
    bool active;
};

inline bool HookInstall(HookCtx* ctx) {
    if (!ctx || !ctx->target || !ctx->detour) return false;

    uintptr_t src = (uintptr_t)ctx->target;
    uintptr_t dst = (uintptr_t)ctx->detour;

    ctx->origLen = 14;

    memcpy(ctx->origBytes, (void*)src, ctx->origLen);

    ctx->original = VirtualAlloc(nullptr, ctx->origLen + 14,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!ctx->original) return false;

    memcpy(ctx->original, ctx->origBytes, ctx->origLen);

    uintptr_t jmpBack = src + ctx->origLen;
    uint8_t* trampJmp = (uint8_t*)ctx->original + ctx->origLen;
    trampJmp[0] = 0x49;
    trampJmp[1] = 0xBB;
    *(uintptr_t*)(trampJmp + 2) = jmpBack;
    trampJmp[10] = 0x41;
    trampJmp[11] = 0xFF;
    trampJmp[12] = 0xE3;

    DWORD oldProtect;
    if (!VirtualProtect((void*)src, ctx->origLen, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        VirtualFree(ctx->original, 0, MEM_RELEASE);
        ctx->original = nullptr;
        return false;
    }

    uint8_t* patch = (uint8_t*)src;
    patch[0] = 0x48;
    patch[1] = 0xB8;
    *(uintptr_t*)(patch + 2) = dst;
    patch[10] = 0xFF;
    patch[11] = 0xE0;

    for (size_t i = 12; i < ctx->origLen; i++)
        patch[i] = 0x90;

    VirtualProtect((void*)src, ctx->origLen, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), (void*)src, ctx->origLen);

    ctx->active = true;
    return true;
}

inline void HookRemove(HookCtx* ctx) {
    if (!ctx || !ctx->active || !ctx->original) return;

    uintptr_t src = (uintptr_t)ctx->target;
    DWORD oldProtect;
    VirtualProtect((void*)src, ctx->origLen, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy((void*)src, ctx->origBytes, ctx->origLen);
    VirtualProtect((void*)src, ctx->origLen, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), (void*)src, ctx->origLen);

    VirtualFree(ctx->original, 0, MEM_RELEASE);
    ctx->active = false;
}
