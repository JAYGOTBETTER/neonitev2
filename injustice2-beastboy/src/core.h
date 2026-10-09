// Small runtime pieces: log, ini, pattern scan, call-site redirect.
#pragma once
#include <windows.h>
#include <cstdint>

namespace bb {

void LogOpen(const char* fileName);
void Log(const char* fmt, ...);

// Directory this .asi was loaded from (Binaries\Retail), with trailing backslash.
const char* ModuleDir();

int  IniInt(const char* section, const char* key, int fallback);
void IniString(const char* section, const char* key, const char* fallback, char* out, DWORD outSize);

// Finds an IDA-style pattern ("48 8B ? ?") in the game executable's code.
uintptr_t FindPattern(const char* pattern);

// Points the E8 call at `site` to `handler` (through a jump stub within 2 GB) and
// returns the function it called before, so the handler can call it.
uintptr_t RedirectCall(uintptr_t site, void* handler);

} // namespace bb
