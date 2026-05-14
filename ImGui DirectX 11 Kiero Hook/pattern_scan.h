#pragma once
#include <Windows.h>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

// ---------------------------------------------------------------------------
// Pattern scanning engine for CS2.
// Parses IDA-style patterns ("48 8B 05 ? ? ? ?") and finds them in module
// memory. Supports RIP-relative resolution for x64 indirect addressing.
// ---------------------------------------------------------------------------

struct Signature {
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> mask;  // 0xFF = match, 0x00 = wildcard

    static Signature FromString(std::string_view pattern) {
        Signature sig;
        std::string cleaned;
        for (auto c : pattern) if (c != ' ') cleaned += c;

        auto nibble = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return -1;
        };

        for (size_t i = 0; i + 1 < cleaned.size(); i += 2) {
            if (cleaned[i] == '?' || cleaned[i + 1] == '?') {
                sig.bytes.push_back(0);
                sig.mask.push_back(0);
            } else {
                int hi = nibble(cleaned[i]);
                int lo = nibble(cleaned[i + 1]);
                sig.bytes.push_back(static_cast<uint8_t>((hi << 4) | lo));
                sig.mask.push_back(0xFF);
            }
        }
        return sig;
    }
};

class PatternScan {
public:
    // Find first match of signature in the given module
    static uintptr_t Find(const Signature& sig, HMODULE module) {
        if (!module || sig.bytes.empty()) return 0;

        auto dos = (const IMAGE_DOS_HEADER*)module;
        auto nt  = (const IMAGE_NT_HEADERS*)((uintptr_t)module + dos->e_lfanew);
        uintptr_t base = (uintptr_t)module;
        size_t imageSize = nt->OptionalHeader.SizeOfImage;

        __try {
            return FindInRange(sig, base, imageSize);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    // Read a 4-byte signed offset at matchAddr + offset
    static int32_t ReadOffset(uintptr_t matchAddr, int32_t offset) {
        return *(int32_t*)(matchAddr + offset);
    }

    // Resolve RIP-relative: [matchAddr + instrOffset] is a 4-byte displ.
    // Target = (matchAddr + instrOffset + 4) + displ
    static uintptr_t ResolveRip(uintptr_t matchAddr, int32_t instrOffset) {
        int32_t displ = *(int32_t*)(matchAddr + instrOffset);
        return (matchAddr + instrOffset + 4) + displ;
    }

    // Convenience: parse pattern, find, read offset value
    static bool FindOffset(const std::string& pattern, const std::string& name,
                           HMODULE mod, int32_t readOff, int32_t& out) {
        auto sig = Signature::FromString(pattern);
        uintptr_t addr = Find(sig, mod);
        if (!addr) {
            printf("[PS] FAIL: %s\n", name.c_str());
            return false;
        }
        out = ReadOffset(addr, readOff);
        printf("[PS] OK: %s = 0x%X\n", name.c_str(), out);
        return true;
    }

    // Convenience: parse pattern, find, resolve RIP-relative pointer
    static bool FindPointer(const std::string& pattern, const std::string& name,
                            HMODULE mod, int32_t instrOff, uintptr_t& out) {
        auto sig = Signature::FromString(pattern);
        uintptr_t addr = Find(sig, mod);
        if (!addr) {
            printf("[PS] FAIL: %s\n", name.c_str());
            return false;
        }
        out = ResolveRip(addr, instrOff);
        printf("[PS] OK: %s = 0x%llX\n", name.c_str(), out);
        return true;
    }

    // -----------------------------------------------------------------------
    // RVA-based scanning: find a RIP-relative instruction or a data pointer
    // that references (module_base + rva).
    // Supports MOV/LEA/MOVSS and scans .data/.rdata for direct QWORD refs.
    // -----------------------------------------------------------------------
    static bool FindPointerByRva(const char* name, HMODULE mod,
                                  std::ptrdiff_t rva, uintptr_t& out) {
        uintptr_t base = (uintptr_t)mod;
        uintptr_t target = base + rva;

        auto dos = (const IMAGE_DOS_HEADER*)mod;
        auto nt  = (const IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
        auto sections = IMAGE_FIRST_SECTION(nt);

        // ---- Scan 1: Find RIP-relative instructions in executable sections ----
        struct InstrForm {
            uint8_t prefix[2];
            int     prefixLen;
            uint8_t opcode[2];
            int     opcodeLen;
            uint8_t modrm;
            int     displOff;
            int     instrLen;
        };

        InstrForm forms[] = {
            { {0x48}, 1, {0x8B}, 1, 0x05, 3, 7 },  // mov rax, [rip+disp]
            { {0x48}, 1, {0x8D}, 1, 0x05, 3, 7 },  // lea rax, [rip+disp]
            { {0x00}, 0, {0x8B}, 1, 0x05, 2, 6 },  // mov eax, [rip+disp]
            { {0x00}, 0, {0x8D}, 1, 0x05, 2, 6 },  // lea eax, [rip+disp]
            { {0xF3, 0x0F}, 2, {0x10}, 1, 0x05, 4, 8 },  // movss xmm0, [rip+disp]
            { {0xF3, 0x0F}, 2, {0x11}, 1, 0x05, 4, 8 },  // movss [rip+disp], xmm0
            { {0xF2, 0x0F}, 2, {0x10}, 1, 0x05, 4, 8 },  // movsd xmm0, [rip+disp]
        };

        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            auto& sec = sections[i];
            if (!(sec.Characteristics & IMAGE_SCN_MEM_EXECUTE))
                continue;

            uint8_t* data = (uint8_t*)(base + sec.VirtualAddress);
            size_t size = sec.SizeOfRawData;

            for (auto& f : forms) {
                for (size_t j = 0; j + (size_t)f.instrLen <= size; ++j) {
                    bool match = true;
                    for (int p = 0; p < f.prefixLen; ++p)
                        if (data[j + p] != f.prefix[p]) { match = false; break; }
                    if (!match) continue;
                    for (int p = 0; p < f.opcodeLen; ++p)
                        if (data[j + f.prefixLen + p] != f.opcode[p]) { match = false; break; }
                    if (!match) continue;
                    if ((data[j + f.prefixLen + f.opcodeLen] & 0xC7) != f.modrm) continue;

                    int32_t displ = *(int32_t*)(data + j + f.displOff);
                    uintptr_t resolved = base + sec.VirtualAddress + j + f.instrLen + displ;

                    if (resolved == target) {
                        out = resolved;
                        printf("[PS-RVA] OK: %s = 0x%llX (instr)\n", name, out);
                        return true;
                    }
                }
            }
        }

        // ---- Scan 2: Data sections - look for QWORD pointers to target ----
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            auto& sec = sections[i];
            if (!(sec.Characteristics & IMAGE_SCN_MEM_READ))
                continue;
            if (sec.Characteristics & IMAGE_SCN_MEM_EXECUTE)
                continue; // skip code, already scanned above

            uint8_t* data = (uint8_t*)(base + sec.VirtualAddress);
            size_t size = sec.SizeOfRawData;

            for (size_t j = 0; j + 8 <= size; j += 8) {
                uintptr_t val = *(uintptr_t*)(data + j);
                if (val == target) {
                    out = base + sec.VirtualAddress + j;
                    printf("[PS-RVA] OK: %s = 0x%llX (data ptr)\n", name, out);
                    return true;
                }
            }
        }

        printf("[PS-RVA] FAIL: %s (RVA 0x%llX)\n", name, (uint64_t)rva);
        return false;
    }

private:
    static uintptr_t FindInRange(const Signature& sig, uintptr_t start, size_t size) {
        if (sig.bytes.empty() || size < sig.bytes.size()) return 0;
        auto data = (const uint8_t*)start;
        for (size_t i = 0; i <= size - sig.bytes.size(); ++i) {
            bool match = true;
            for (size_t j = 0; j < sig.bytes.size(); ++j) {
                if (sig.mask[j] == 0xFF && data[i + j] != sig.bytes[j]) {
                    match = false;
                    break;
                }
            }
            if (match) return start + i;
        }
        return 0;
    }
};
