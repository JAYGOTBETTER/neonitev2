#include "core.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <vector>

namespace bb {

static FILE* g_log = nullptr;
static char  g_dir[MAX_PATH] = {};
static char  g_ini[MAX_PATH] = {};

const char* ModuleDir()
{
    if (!g_dir[0]) {
        HMODULE self = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(&ModuleDir), &self);
        GetModuleFileNameA(self, g_dir, MAX_PATH);
        if (char* slash = strrchr(g_dir, '\\'))
            slash[1] = 0;
        snprintf(g_ini, sizeof(g_ini), "%sBeastBoy.ini", g_dir);
    }
    return g_dir;
}

void LogOpen(const char* fileName)
{
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s%s", ModuleDir(), fileName);
    g_log = fopen(path, "w");
}

void Log(const char* fmt, ...)
{
    if (!g_log)
        return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    fprintf(g_log, "[%02d:%02d:%02d.%03d] ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    va_list args;
    va_start(args, fmt);
    vfprintf(g_log, fmt, args);
    va_end(args);
    fputc('\n', g_log);
    fflush(g_log);
}

int IniInt(const char* section, const char* key, int fallback)
{
    ModuleDir();
    return (int)GetPrivateProfileIntA(section, key, fallback, g_ini);
}

void IniString(const char* section, const char* key, const char* fallback, char* out, DWORD outSize)
{
    ModuleDir();
    GetPrivateProfileStringA(section, key, fallback, out, outSize, g_ini);
}

uintptr_t FindPattern(const char* pattern)
{
    std::vector<int> bytes; // -1 = wildcard
    for (const char* p = pattern; *p;) {
        if (*p == ' ') { ++p; continue; }
        if (*p == '?') { bytes.push_back(-1); while (*p == '?') ++p; continue; }
        char hex[3] = { p[0], p[1], 0 };
        bytes.push_back((int)strtol(hex, nullptr, 16));
        p += 2;
    }

    auto base = reinterpret_cast<uint8_t*>(GetModuleHandleA(nullptr));
    auto dos  = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt   = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    auto sec  = IMAGE_FIRST_SECTION(nt);
    const size_t n = bytes.size();

    for (unsigned s = 0; s < nt->FileHeader.NumberOfSections; ++s, ++sec) {
        if (!(sec->Characteristics & IMAGE_SCN_MEM_EXECUTE))
            continue;
        uint8_t* start = base + sec->VirtualAddress;
        size_t size = sec->Misc.VirtualSize;
        if (size < n)
            continue;
        for (size_t i = 0; i <= size - n; ++i) {
            size_t j = 0;
            while (j < n && (bytes[j] < 0 || start[i + j] == (uint8_t)bytes[j]))
                ++j;
            if (j == n)
                return reinterpret_cast<uintptr_t>(start + i);
        }
    }
    return 0;
}

// A page of 14-byte absolute jumps placed within +-2 GB of the game image.
static uint8_t* g_stubs = nullptr;
static size_t   g_stubUsed = 0;

static uint8_t* NearStub(uintptr_t anchor, void* target)
{
    if (!g_stubs) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        const uintptr_t gran = si.dwAllocationGranularity;
        uintptr_t lo = anchor > 0x7FF00000ull ? anchor - 0x7FF00000ull : gran;
        uintptr_t hi = anchor + 0x7FF00000ull;
        for (uintptr_t a = (lo + gran - 1) / gran * gran; a < hi && !g_stubs; a += gran) {
            MEMORY_BASIC_INFORMATION mbi;
            if (!VirtualQuery(reinterpret_cast<void*>(a), &mbi, sizeof(mbi)))
                continue;
            if (mbi.State == MEM_FREE)
                g_stubs = static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(a), 0x1000,
                                                              MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
        }
        if (!g_stubs)
            return nullptr;
    }
    if (g_stubUsed + 16 > 0x1000)
        return nullptr;
    uint8_t* stub = g_stubs + g_stubUsed;
    g_stubUsed += 16;
    stub[0] = 0xFF; stub[1] = 0x25;               // jmp [rip+0]
    memset(stub + 2, 0, 4);
    memcpy(stub + 6, &target, sizeof(target));
    return stub;
}

uintptr_t RedirectCall(uintptr_t site, void* handler)
{
    auto code = reinterpret_cast<uint8_t*>(site);
    if (code[0] != 0xE8)
        return 0;
    int32_t rel;
    memcpy(&rel, code + 1, 4);
    uintptr_t original = site + 5 + (intptr_t)rel;

    uint8_t* stub = NearStub(site, handler);
    if (!stub)
        return 0;
    int64_t newRel = reinterpret_cast<intptr_t>(stub) - (intptr_t)(site + 5);
    if (newRel > INT32_MAX || newRel < INT32_MIN)
        return 0;

    DWORD old;
    VirtualProtect(code + 1, 4, PAGE_EXECUTE_READWRITE, &old);
    int32_t rel32 = (int32_t)newRel;
    memcpy(code + 1, &rel32, 4);
    VirtualProtect(code + 1, 4, old, &old);
    FlushInstructionCache(GetCurrentProcess(), code, 5);
    return original;
}

} // namespace bb
