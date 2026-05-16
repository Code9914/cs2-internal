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
    printf("[MM] Opening DLL: %s\n", dllPath.c_str());
    std::ifstream file(dllPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) { printf("[MM] FAILED to open DLL\n"); return false; }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> fileBuffer(size);
    if (!file.read(fileBuffer.data(), size)) { printf("[MM] FAILED to read DLL\n"); return false; }
    file.close();
    printf("[MM] DLL loaded: %zu bytes\n", size);

    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)fileBuffer.data();
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) { printf("[MM] Invalid DOS header\n"); return false; }
    IMAGE_NT_HEADERS* fileNtHeaders = (IMAGE_NT_HEADERS*)(fileBuffer.data() + dosHeader->e_lfanew);
    if (fileNtHeaders->Signature != IMAGE_NT_SIGNATURE) { printf("[MM] Invalid NT header\n"); return false; }
    DWORD imageSize = fileNtHeaders->OptionalHeader.SizeOfImage;
    printf("[MM] ImageSize: 0x%X, Sections: %d\n", imageSize, fileNtHeaders->FileHeader.NumberOfSections);

    // Map file into a local image buffer at virtual addresses (like the remote process will have)
    std::vector<char> localImage(imageSize, 0);
    memcpy(localImage.data(), fileBuffer.data(), fileNtHeaders->OptionalHeader.SizeOfHeaders);
    IMAGE_SECTION_HEADER* fileSection = IMAGE_FIRST_SECTION(fileNtHeaders);
    for (WORD i = 0; i < fileNtHeaders->FileHeader.NumberOfSections; ++i, ++fileSection) {
        if (fileSection->SizeOfRawData) {
            memcpy(localImage.data() + fileSection->VirtualAddress,
                fileBuffer.data() + fileSection->PointerToRawData, fileSection->SizeOfRawData);
        }
    }
    printf("[MM] Local image mapped\n");

    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(localImage.data() + dosHeader->e_lfanew);

    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!process) { printf("[MM] OpenProcess failed: %lu\n", GetLastError()); return false; }
    printf("[MM] Process opened\n");

    void* remoteBase = VirtualAllocEx(process, nullptr, imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!remoteBase) { printf("[MM] VirtualAllocEx failed: %lu\n", GetLastError()); CloseHandle(process); return false; }
    printf("[MM] Remote memory allocated at: 0x%p\n", remoteBase);

    SIZE_T written;
    if (!WriteProcessMemory(process, remoteBase, localImage.data(), ntHeaders->OptionalHeader.SizeOfHeaders, &written)) {
        printf("[MM] Write headers failed: %lu\n", GetLastError());
        VirtualFreeEx(process, remoteBase, 0, MEM_RELEASE); CloseHandle(process); return false;
    }
    printf("[MM] Headers written: %zu bytes\n", written);

    IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i, ++section) {
        if (section->SizeOfRawData) {
            if (!WriteProcessMemory(process, (void*)((uintptr_t)remoteBase + section->VirtualAddress),
                fileBuffer.data() + section->PointerToRawData, section->SizeOfRawData, &written)) {
                printf("[MM] Write section %s failed: %lu\n", section->Name, GetLastError());
                VirtualFreeEx(process, remoteBase, 0, MEM_RELEASE); CloseHandle(process); return false;
            }
            printf("[MM] Section %s written: %zu bytes at +0x%X\n", section->Name, written, section->VirtualAddress);
        }
    }

    // Fix imports - parse from LOCAL buffer, write resolved addresses to remote process
    IMAGE_DATA_DIRECTORY& importDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    printf("[MM] Fixing imports... (VA=0x%X, Size=0x%X)\n", importDir.VirtualAddress, importDir.Size);
    if (importDir.VirtualAddress) {
        IMAGE_IMPORT_DESCRIPTOR* importDesc = (IMAGE_IMPORT_DESCRIPTOR*)(localImage.data() + importDir.VirtualAddress);
        int modCount = 0;
        for (; importDesc->Name; ++importDesc) {
            const char* modName = localImage.data() + importDesc->Name;
            HMODULE hMod = LoadLibraryA(modName);
            if (!hMod) { printf("[MM] LoadLibrary %s failed\n", modName); modCount++; continue; }

            uintptr_t intRVA = importDesc->OriginalFirstThunk ? importDesc->OriginalFirstThunk : importDesc->FirstThunk;
            IMAGE_THUNK_DATA64* intEntries = (IMAGE_THUNK_DATA64*)(localImage.data() + intRVA);

            int funcCount = 0;
            int idx = 0;
            for (; intEntries[idx].u1.AddressOfData; ++idx) {
                FARPROC funcAddr = nullptr;
                if (IMAGE_SNAP_BY_ORDINAL64(intEntries[idx].u1.Ordinal)) {
                    funcAddr = GetProcAddress(hMod, (LPCSTR)IMAGE_ORDINAL64(intEntries[idx].u1.Ordinal));
                } else {
                    IMAGE_IMPORT_BY_NAME* nameData = (IMAGE_IMPORT_BY_NAME*)(localImage.data() + intEntries[idx].u1.AddressOfData);
                    funcAddr = GetProcAddress(hMod, nameData->Name);
                }
                if (funcAddr) {
                    ULONGLONG funcPtr = (ULONGLONG)funcAddr;
                    uintptr_t remoteIAT = (uintptr_t)remoteBase + importDesc->FirstThunk + idx * sizeof(ULONGLONG);
                    WriteProcessMemory(process, (void*)remoteIAT, &funcPtr, sizeof(funcPtr), nullptr);
                    funcCount++;
                }
            }
            modCount++;
            if (modCount <= 5 || modCount % 5 == 0)
                printf("[MM]   %s: %d functions resolved\n", modName, funcCount);
        }
        printf("[MM] Imports fixed: %d modules\n", modCount);
    } else {
        printf("[MM] No imports\n");
    }

    // Fix relocations - parse from LOCAL buffer
    IMAGE_DATA_DIRECTORY& relocDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    printf("[MM] Fixing relocations... (VA=0x%X, Size=0x%X)\n", relocDir.VirtualAddress, relocDir.Size);
    if (relocDir.VirtualAddress && relocDir.Size) {
        uintptr_t delta = (uintptr_t)remoteBase - ntHeaders->OptionalHeader.ImageBase;
        printf("[MM] Delta: 0x%llX\n", (unsigned long long)delta);
        IMAGE_BASE_RELOCATION* reloc = (IMAGE_BASE_RELOCATION*)(localImage.data() + relocDir.VirtualAddress);
        IMAGE_BASE_RELOCATION* relocEnd = (IMAGE_BASE_RELOCATION*)(localImage.data() + relocDir.VirtualAddress + relocDir.Size);
        int relocCount = 0;
        while (reloc < relocEnd && reloc->VirtualAddress && reloc->SizeOfBlock) {
            if (reloc->SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION)) break;
            if ((char*)reloc + reloc->SizeOfBlock > (char*)relocEnd) break;
            DWORD count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            WORD* entries = (WORD*)(reloc + 1);
            for (DWORD i = 0; i < count; ++i) {
                WORD type = entries[i] >> 12;
                WORD offset = entries[i] & 0xFFF;
                if (type == IMAGE_REL_BASED_DIR64) {
                    ULONGLONG* target = (ULONGLONG*)((uintptr_t)remoteBase + reloc->VirtualAddress + offset);
                    ULONGLONG value;
                    if (!ReadProcessMemory(process, target, &value, sizeof(value), nullptr)) continue;
                    value += delta;
                    WriteProcessMemory(process, target, &value, sizeof(value), nullptr);
                    relocCount++;
                }
            }
            reloc = (IMAGE_BASE_RELOCATION*)((char*)reloc + reloc->SizeOfBlock);
        }
        printf("[MM] Relocations fixed: %d\n", relocCount);
    } else {
        printf("[MM] No relocations\n");
    }

    // Call DllMain with shellcode
    printf("[MM] Calling DllMain...\n");
    uintptr_t entryPoint = (uintptr_t)remoteBase + ntHeaders->OptionalHeader.AddressOfEntryPoint;
    printf("[MM] Entry point: 0x%p\n", (void*)entryPoint);

    unsigned char shellcode[] = {
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x48, 0xBA, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x4D, 0x31, 0xC0,
        0xFF, 0xE0
    };
    memcpy(shellcode + 2, &entryPoint, 8);
    memcpy(shellcode + 12, &remoteBase, 8);

    void* shellcodeMem = VirtualAllocEx(process, nullptr, sizeof(shellcode), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!shellcodeMem) { printf("[MM] Shellcode alloc failed: %lu\n", GetLastError()); CloseHandle(process); return false; }
    WriteProcessMemory(process, shellcodeMem, shellcode, sizeof(shellcode), nullptr);
    printf("[MM] Shellcode written at: 0x%p\n", shellcodeMem);

    HANDLE thread = CreateRemoteThread(process, nullptr, 0, (LPTHREAD_START_ROUTINE)shellcodeMem, nullptr, 0, nullptr);
    if (!thread) { printf("[MM] CreateRemoteThread failed: %lu\n", GetLastError()); CloseHandle(process); return false; }

    WaitForSingleObject(thread, 5000);
    DWORD exitCode = 0;
    GetExitCodeThread(thread, &exitCode);
    printf("[MM] DllMain exit code: 0x%X\n", exitCode);
    CloseHandle(thread);
    CloseHandle(process);

    printf("[MM] Manual mapping complete! Base: 0x%p\n", remoteBase);
    return true;
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
