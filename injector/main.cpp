#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#pragma comment(lib, "ntdll.lib")

typedef LONG NTSTATUS;
typedef NTSTATUS(NTAPI* _NtCreateThreadEx)(
    PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess,
    PVOID ObjectAttributes, HANDLE ProcessHandle,
    PVOID StartRoutine, PVOID Argument, ULONG CreateFlags,
    SIZE_T ZeroBits, SIZE_T StackSize, SIZE_T MaximumStackSize, PVOID AttributeList);

typedef NTSTATUS(NTAPI* _RtlAdjustPrivilege)(
    ULONG Privilege, BOOLEAN Enable, BOOLEAN CurrentThread, PBOOLEAN Enabled);

DWORD FindProcessId(const std::string& name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name.c_str()) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

bool SetDebugPrivilege() {
    BOOLEAN enabled = FALSE;
    auto RtlAdjustPrivilege = (_RtlAdjustPrivilege)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "RtlAdjustPrivilege");
    if (!RtlAdjustPrivilege) return false;
    return RtlAdjustPrivilege(20, TRUE, FALSE, &enabled) >= 0;
}

// --- Manual Mapping Injector ---
bool ManualMap(DWORD pid, const std::string& dllPath) {
    std::ifstream file(dllPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) { std::cerr << "  Failed to open DLL" << std::endl; return false; }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) { std::cerr << "  Failed to read DLL" << std::endl; return false; }
    file.close();

    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)buffer.data();
    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(buffer.data() + dosHeader->e_lfanew);
    DWORD imageSize = ntHeaders->OptionalHeader.SizeOfImage;

    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!process) { std::cerr << "  OpenProcess failed: " << GetLastError() << std::endl; return false; }

    void* remoteBase = VirtualAllocEx(process, nullptr, imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!remoteBase) { std::cerr << "  VirtualAllocEx failed" << std::endl; CloseHandle(process); return false; }

    // Write headers
    WriteProcessMemory(process, remoteBase, buffer.data(), ntHeaders->OptionalHeader.SizeOfHeaders, nullptr);

    // Write sections
    IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i, ++section) {
        if (section->SizeOfRawData) {
            WriteProcessMemory(process, (void*)((uintptr_t)remoteBase + section->VirtualAddress),
                buffer.data() + section->PointerToRawData, section->SizeOfRawData, nullptr);
        }
    }

    // Fix imports
    IMAGE_DATA_DIRECTORY& importDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.VirtualAddress) {
        IMAGE_IMPORT_DESCRIPTOR* importDesc = (IMAGE_IMPORT_DESCRIPTOR*)((uintptr_t)remoteBase + importDir.VirtualAddress);
        for (; importDesc->Name; ++importDesc) {
            const char* modName = (const char*)((uintptr_t)remoteBase + importDesc->Name);
            HMODULE hMod = LoadLibraryA(modName);
            if (!hMod) continue;

            IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)((uintptr_t)remoteBase + importDesc->FirstThunk);
            IMAGE_THUNK_DATA* origThunk = (IMAGE_THUNK_DATA*)((uintptr_t)remoteBase + importDesc->OriginalFirstThunk);
            for (; origThunk->u1.AddressOfData; ++thunk, ++origThunk) {
                if (IMAGE_SNAP_BY_ORDINAL(origThunk->u1.Ordinal)) {
                    thunk->u1.Function = (ULONGLONG)GetProcAddress(hMod, (LPCSTR)IMAGE_ORDINAL(origThunk->u1.Ordinal));
                } else {
                    IMAGE_IMPORT_BY_NAME* nameData = (IMAGE_IMPORT_BY_NAME*)((uintptr_t)remoteBase + origThunk->u1.AddressOfData);
                    thunk->u1.Function = (ULONGLONG)GetProcAddress(hMod, nameData->Name);
                }
            }
        }
    }

    // Fix relocations
    IMAGE_DATA_DIRECTORY& relocDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (relocDir.VirtualAddress) {
        uintptr_t delta = (uintptr_t)remoteBase - ntHeaders->OptionalHeader.ImageBase;
        IMAGE_BASE_RELOCATION* reloc = (IMAGE_BASE_RELOCATION*)((uintptr_t)remoteBase + relocDir.VirtualAddress);
        while (reloc->VirtualAddress) {
            DWORD count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            WORD* entries = (WORD*)(reloc + 1);
            for (DWORD i = 0; i < count; ++i) {
                WORD type = entries[i] >> 12;
                WORD offset = entries[i] & 0xFFF;
                if (type == IMAGE_REL_BASED_DIR64) {
                    ULONGLONG* target = (ULONGLONG*)((uintptr_t)remoteBase + reloc->VirtualAddress + offset);
                    ULONGLONG value;
                    ReadProcessMemory(process, target, &value, sizeof(value), nullptr);
                    value += delta;
                    WriteProcessMemory(process, target, &value, sizeof(value), nullptr);
                }
            }
            reloc = (IMAGE_BASE_RELOCATION*)((char*)reloc + reloc->SizeOfBlock);
        }
    }

    // Call DllMain
    uintptr_t entryPoint = (uintptr_t)remoteBase + ntHeaders->OptionalHeader.AddressOfEntryPoint;
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, (LPTHREAD_START_ROUTINE)entryPoint, remoteBase, 0, nullptr);
    if (!thread) { std::cerr << "  CreateRemoteThread failed" << std::endl; CloseHandle(process); return false; }

    WaitForSingleObject(thread, INFINITE);
    DWORD exitCode;
    GetExitCodeThread(thread, &exitCode);
    CloseHandle(thread);
    CloseHandle(process);

    std::cout << "  Manual mapped! Base: 0x" << std::hex << (uintptr_t)remoteBase << " Entry: 0x" << exitCode << std::dec << std::endl;
    return exitCode != 0;
}

bool EjectDLL(DWORD pid, const std::string& dllName) {
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!process) { std::cerr << "  OpenProcess failed" << std::endl; return false; }

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) { CloseHandle(process); return false; }

    MODULEENTRY32 me = { sizeof(me) };
    HMODULE targetMod = nullptr;
    if (Module32First(snap, &me)) {
        do {
            if (_stricmp(me.szModule, dllName.c_str()) == 0 || _stricmp(me.szExePath, dllName.c_str()) == 0) {
                targetMod = me.hModule; break;
            }
        } while (Module32Next(snap, &me));
    }
    CloseHandle(snap);
    if (!targetMod) { CloseHandle(process); return false; }

    LPTHREAD_START_ROUTINE freeLib = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FreeLibrary");
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, freeLib, targetMod, 0, nullptr);
    if (!thread) { CloseHandle(process); return false; }
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread); CloseHandle(process);
    std::cout << "  Ejected!" << std::endl;
    return true;
}

void PrintUsage() {
    std::cout << "CS2 Injector (Manual Map)" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  inject <dll_path>                 Inject into cs2.exe" << std::endl;
    std::cout << "  inject <dll_path> --pid <pid>     Inject into specific PID" << std::endl;
    std::cout << "  inject --eject <dll_name> [--pid] Eject DLL" << std::endl;
    std::cout << "  inject --help                     This help" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string targetProcess = "cs2.exe";
    DWORD pid = 0;
    std::string dllPath, dllName;
    bool eject = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") { PrintUsage(); return 0; }
        else if (arg == "--eject") { eject = true; if (i + 1 < argc) dllName = argv[++i]; }
        else if (arg == "--pid") { if (i + 1 < argc) pid = std::stoul(argv[++i]); }
        else if (dllPath.empty() && !eject) dllPath = arg;
        else dllName = arg;
    }

    SetDebugPrivilege();

    if (eject) {
        if (!pid) pid = FindProcessId(targetProcess);
        if (!pid) { std::cerr << "Process not found" << std::endl; return 1; }
        std::cout << "PID: " << pid << std::endl;
        return EjectDLL(pid, dllName) ? 0 : 1;
    }

    if (dllPath.empty()) { PrintUsage(); return 1; }
    if (!pid) pid = FindProcessId(targetProcess);
    if (!pid) { std::cerr << "cs2.exe not found" << std::endl; return 1; }

    std::cout << "PID: " << pid << "  Method: ManualMap" << std::endl;
    std::cout << "DLL: " << dllPath << std::endl;

    return ManualMap(pid, dllPath) ? 0 : 1;
}
