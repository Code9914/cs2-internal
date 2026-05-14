#pragma once
#include "includes.h"

// Schema System — vtable layout: index 13 = FindTypeScopeForModule, scope index 2 = FindDeclaredClass
// Class info layout (from runtime analysis): fields @ +0x30, fieldCount @ +0x24 (high32)

class CSchemaClassInfo {};

class CSchemaSystemTypeScope {
public:
    CSchemaClassInfo* FindDeclaredClass(const char* name) {
        __try {
            char temp;
            using Fn = uintptr_t(__fastcall*)(CSchemaSystemTypeScope*, char*, const char*);
            uintptr_t result = (*(Fn**)(uintptr_t*)this)[2](this, &temp, name);
            if (result) return *(CSchemaClassInfo**)result;
        } __except(EXCEPTION_EXECUTE_HANDLER) {}
        return nullptr;
    }
};

class CSchemaSystem {
public:
    CSchemaSystemTypeScope* FindTypeScopeForModule(const char* name, int flags = 0) {
        __try {
            using Fn = CSchemaSystemTypeScope * (__fastcall*)(CSchemaSystem*, const char*, int);
            return (*(Fn**)(uintptr_t*)this)[13](this, name, flags);
        } __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }
};

inline CSchemaSystem* g_SchemaSystem = nullptr;

inline bool InitSchemaSystem(CreateInterfaceFn fn) {
    if (g_SchemaSystem) return true;
    __try {
        g_SchemaSystem = (CSchemaSystem*)fn("SchemaSystem_001", nullptr);
        if (!g_SchemaSystem || (uintptr_t)g_SchemaSystem <= 0x100000) return false;
        auto scope = g_SchemaSystem->FindTypeScopeForModule("client.dll");
        if (!scope) return false;
        if (!scope->FindDeclaredClass("C_BaseEntity")) return false;
        printf("[Schema] OK\n");
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        g_SchemaSystem = nullptr;
        return false;
    }
}

inline int32_t ResolveSchemaOffset(const char* className, const char* fieldName,
                                    int32_t defaultOffset) {
    if (!g_SchemaSystem) return defaultOffset;

    __try {
        auto scope = g_SchemaSystem->FindTypeScopeForModule("client.dll");
        if (!scope) return defaultOffset;

        auto cls = scope->FindDeclaredClass(className);
        if (!cls) return defaultOffset;

        uintptr_t addr = (uintptr_t)cls;
        uintptr_t fields = *(uintptr_t*)(addr + 0x30);
        int32_t fieldCount = (int32_t)(*(uint64_t*)(addr + 0x20) >> 32);

        if (!fields || fields < 0x100000 || fieldCount <= 0 || fieldCount > 5000)
            return defaultOffset;

        for (int i = 0; i < fieldCount; i++) {
            uintptr_t field = fields + i * 0x18;
            const char* name = *(const char**)field;
            if (name && (uintptr_t)name > 0x100000) {
                __try {
                    if (_stricmp(name, fieldName) == 0) {
                        int32_t offset = *(int32_t*)(field + 0x10);
                        printf("[Schema] %s::%s = 0x%X\n", className, fieldName, offset);
                        return offset;
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {}
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    return defaultOffset;
}
