#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <psapi.h>

#pragma comment(lib, "ntdll.lib")

// --- NT API types ---
typedef LONG NTSTATUS;
typedef struct _UNICODE_STRING {
    USHORT Length; USHORT MaximumLength; PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef NTSTATUS(NTAPI* _NtCreateThreadEx)(
    PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess,
    PVOID ObjectAttributes, HANDLE ProcessHandle,
    PVOID StartRoutine, PVOID Argument, ULONG CreateFlags,
    SIZE_T ZeroBits, SIZE_T StackSize, SIZE_T MaximumStackSize, PVOID AttributeList);

typedef NTSTATUS(NTAPI* _NtSetInformationThread)(
    HANDLE ThreadHandle, ULONG ThreadInformationClass,
    PVOID ThreadInformation, ULONG ThreadInformationLength);

typedef NTSTATUS(NTAPI* _RtlAdjustPrivilege)(
    ULONG Privilege, BOOLEAN Enable, BOOLEAN CurrentThread, PBOOLEAN Enabled);

// --- Utils ---
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
    return RtlAdjustPrivilege(20 /* SeDebugPrivilege */, TRUE, FALSE, &enabled) >= 0;
}

// --- Injection via NtCreateThreadEx ---
bool InjectNT(DWORD pid, const std::string& dllPath) {
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!process) { std::cerr << "  OpenProcess failed: " << GetLastError() << std::endl; return false; }

    void* remoteMem = VirtualAllocEx(process, nullptr, dllPath.size() + 1,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        std::cerr << "  VirtualAllocEx failed: " << GetLastError() << std::endl;
        CloseHandle(process); return false;
    }

    if (!WriteProcessMemory(process, remoteMem, dllPath.c_str(), dllPath.size() + 1, nullptr)) {
        std::cerr << "  WriteProcessMemory failed: " << GetLastError() << std::endl;
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE); CloseHandle(process); return false;
    }

    auto NtCreateThreadEx = (_NtCreateThreadEx)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtCreateThreadEx");
    if (!NtCreateThreadEx) {
        std::cerr << "  NtCreateThreadEx not found" << std::endl;
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE); CloseHandle(process); return false;
    }

    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    LPTHREAD_START_ROUTINE loadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryA");

    HANDLE thread = NULL;
    NTSTATUS status = NtCreateThreadEx(&thread, THREAD_ALL_ACCESS, nullptr, process,
        loadLib, remoteMem, 0, 0, 0, 0, nullptr);

    if (status < 0 || !thread) {
        std::cerr << "  NtCreateThreadEx failed: 0x" << std::hex << status << std::dec << std::endl;
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE); CloseHandle(process); return false;
    }

    // Hide thread from debugger
    auto NtSetInformationThread = (_NtSetInformationThread)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");
    if (NtSetInformationThread)
        NtSetInformationThread(thread, 0x11 /* ThreadHideFromDebugger */, nullptr, 0);

    WaitForSingleObject(thread, INFINITE);
    DWORD exitCode;
    GetExitCodeThread(thread, &exitCode);

    VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
    CloseHandle(thread);
    CloseHandle(process);

    if (!exitCode) {
        std::cerr << "  LoadLibrary returned NULL" << std::endl;
        return false;
    }
    std::cout << "  Injected! DLL base: 0x" << std::hex << exitCode << std::dec << std::endl;
    return true;
}

// --- Injection via thread hijacking ---
bool InjectHijack(DWORD pid, const std::string& dllPath) {
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!process) return false;

    void* remoteMem = VirtualAllocEx(process, nullptr, dllPath.size() + 1,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) { CloseHandle(process); return false; }
    if (!WriteProcessMemory(process, remoteMem, dllPath.c_str(), dllPath.size() + 1, nullptr)) {
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE); CloseHandle(process); return false;
    }

    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    LPTHREAD_START_ROUTINE loadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryA");

    // First thread in the process
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) { CloseHandle(process); return false; }

    THREADENTRY32 te = { sizeof(te) };
    DWORD targetTid = 0;
    if (Thread32First(snapshot, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) { targetTid = te.th32ThreadID; break; }
        } while (Thread32Next(snapshot, &te));
    }
    CloseHandle(snapshot);

    if (!targetTid) { CloseHandle(process); return false; }

    HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, targetTid);
    if (!thread) { CloseHandle(process); return false; }

    SuspendThread(thread);

    CONTEXT ctx = { CONTEXT_CONTROL };
    if (!GetThreadContext(thread, &ctx)) {
        ResumeThread(thread); CloseHandle(thread); CloseHandle(process); return false;
    }

#ifdef _WIN64
    ctx.Rsp -= 8;
    ctx.Rip = (DWORD64)loadLib;
    ctx.Rcx = (DWORD64)remoteMem;
#else
    ctx.Esp -= 8;
    ctx.Eip = (DWORD)loadLib;
    ctx.Ecx = (DWORD)remoteMem;
#endif

    if (!SetThreadContext(thread, &ctx)) {
        ResumeThread(thread); CloseHandle(thread); CloseHandle(process); return false;
    }

    ResumeThread(thread);

    // Wait briefly
    if (WaitForSingleObject(thread, 3000) == WAIT_OBJECT_0) {
        DWORD exitCode;
        GetExitCodeThread(thread, &exitCode);
        std::cout << "  Hijack injected! DLL base: 0x" << std::hex << exitCode << std::dec << std::endl;
        CloseHandle(thread); CloseHandle(process); return true;
    }

    // Thread didn't finish (normal for game threads), but injection likely worked
    std::cout << "  Hijack injection done (async)" << std::endl;
    ResumeThread(thread);
    CloseHandle(thread);
    CloseHandle(process);
    return true;
}

bool EjectDLL(DWORD pid, const std::string& dllName) {
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!process) { std::cerr << "  OpenProcess failed: " << GetLastError() << std::endl; return false; }

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) { CloseHandle(process); return false; }

    MODULEENTRY32 me = { sizeof(me) };
    HMODULE targetMod = nullptr;
    if (Module32First(snap, &me)) {
        do {
            if (_stricmp(me.szModule, dllName.c_str()) == 0 ||
                _stricmp(me.szExePath, dllName.c_str()) == 0) {
                targetMod = me.hModule; break;
            }
        } while (Module32Next(snap, &me));
    }
    CloseHandle(snap);

    if (!targetMod) { std::cerr << "  DLL not found in target" << std::endl; CloseHandle(process); return false; }

    LPTHREAD_START_ROUTINE freeLib = (LPTHREAD_START_ROUTINE)GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "FreeLibrary");

    auto NtCreateThreadEx = (_NtCreateThreadEx)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtCreateThreadEx");

    HANDLE thread = NULL;
    if (NtCreateThreadEx) {
        NtCreateThreadEx(&thread, THREAD_ALL_ACCESS, nullptr, process,
            freeLib, targetMod, 0, 0, 0, 0, nullptr);
    }
    if (!thread) {
        thread = CreateRemoteThread(process, nullptr, 0, freeLib, targetMod, 0, nullptr);
    }
    if (!thread) { CloseHandle(process); return false; }

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    CloseHandle(process);
    std::cout << "  Ejected!" << std::endl;
    return true;
}

void PrintUsage() {
    std::cout << "CS2 Injector" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  inject <dll_path>                 Inject into cs2.exe" << std::endl;
    std::cout << "  inject <dll_path> --pid <pid>     Inject into specific PID" << std::endl;
    std::cout << "  inject --eject <dll_name> [--pid] Eject DLL" << std::endl;
    std::cout << "  inject --method [nt|hijack] ...   Select injection method" << std::endl;
    std::cout << "  inject --help                     This help" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string targetProcess = "cs2.exe";
    std::string method = "nt";
    DWORD pid = 0;
    std::string dllPath, dllName;
    bool eject = false;

    // Parse args
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") { PrintUsage(); return 0; }
        else if (arg == "--eject") { eject = true; if (i + 1 < argc) dllName = argv[++i]; }
        else if (arg == "--pid") { if (i + 1 < argc) pid = std::stoul(argv[++i]); }
        else if (arg == "--method") { if (i + 1 < argc) method = argv[++i]; }
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

    std::cout << "PID: " << pid << "  Method: " << method << std::endl;
    std::cout << "DLL: " << dllPath << std::endl;

    if (method == "hijack") {
        if (InjectHijack(pid, dllPath)) return 0;
        std::cout << "Hijack failed, falling back to NtCreateThreadEx..." << std::endl;
    }

    if (InjectNT(pid, dllPath)) return 0;

    std::cerr << "All injection methods failed" << std::endl;
    return 1;
}
