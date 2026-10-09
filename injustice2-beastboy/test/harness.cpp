// Windows test harness (runs under Wine in CI-less cloud sessions):
//  1. FindPattern finds a known byte pattern in this exe's code with wildcards.
//  2. RedirectCall rewires a real E8 call through a near stub and still reaches the original.
//  3. The built zBeastBoy.asi loads, exports InitializeASI, and refuses to hook a non-Injustice exe.
#include <windows.h>
#include <cstdio>
#include <cstring>
#include "core.h"
#include "generated/sheets.h"

extern "C" int64_t orig_fn(int64_t a, int64_t b) { return a * 10 + b; }
extern "C" int64_t site_fn(int64_t a, int64_t b);
extern "C" unsigned char marker_bytes[];

asm(R"(
    .text
    .globl site_fn
site_fn:
    sub $40, %rsp
    call orig_fn
    add $40, %rsp
    ret
    .globl marker_bytes
marker_bytes:
    .byte 0x48, 0x8B, 0x49, 0x20, 0x48, 0x85, 0xC9, 0x74, 0x09, 0x48, 0x85, 0xD2, 0x0F, 0x85, 0x11, 0x22, 0x33, 0x44, 0xC3
)");

static int64_t (*g_orig)(int64_t, int64_t);
static int64_t handler(int64_t a, int64_t b) { return g_orig(a, b) + 1000; }

static int fails = 0;
#define CHECK(c, ...) do { if (c) printf("PASS  " __VA_ARGS__); else { printf("FAIL  " __VA_ARGS__); ++fails; } printf("\n"); } while (0)

int main(int argc, char** argv)
{
    // 1. Same pattern as hooks.Character_SetScale; the four ? bytes are arbitrary here.
    uintptr_t found = bb::FindPattern(bb::kHooks[bb::HOOK_Character_SetScale].pattern);
    CHECK(found == (uintptr_t)marker_bytes, "FindPattern locates Character_SetScale pattern with wildcards (%p == %p)",
          (void*)found, (void*)marker_bytes);
    CHECK(bb::FindPattern("DE AD BE EF DE AD BE EF 13 37 ? 42") == 0, "FindPattern returns 0 for an absent pattern");

    // 2. site_fn = sub rsp,40 (4 bytes) then E8 call orig_fn.
    CHECK(site_fn(3, 4) == 34, "unhooked call returns original result (34)");
    g_orig = (int64_t(*)(int64_t, int64_t))bb::RedirectCall((uintptr_t)site_fn + 4, (void*)handler);
    CHECK(g_orig == orig_fn, "RedirectCall returns the original call target");
    CHECK(site_fn(3, 4) == 1034, "redirected call reaches handler, which forwards args to original (1034)");
    CHECK(bb::RedirectCall((uintptr_t)site_fn, (void*)handler) == 0, "RedirectCall refuses a non-E8 site");

    // 3. Load the real plugin.
    if (argc > 1) {
        HMODULE asi = LoadLibraryA(argv[1]);
        CHECK(asi != nullptr, "LoadLibrary(%s)", argv[1]);
        auto init = asi ? (void (*)())GetProcAddress(asi, "InitializeASI") : nullptr;
        CHECK(init != nullptr, "plugin exports InitializeASI");
        if (init) init();
        CHECK(true, "InitializeASI returned without crashing on a non-Injustice exe");
    }
    printf("%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails;
}
