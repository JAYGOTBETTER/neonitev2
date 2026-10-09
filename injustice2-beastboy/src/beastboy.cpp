// Beast Boy for Injustice 2: whoever picks Cheetah fights as Beast Boy, bathed in
// green light, and his assist button morphs him into Gorilla Grodd for a slam.
#include <windows.h>
#include <xinput.h>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include "core.h"
#include "game.h"

using namespace bb;

namespace {

constexpr int kSlots = 2; // P1, P2

struct Settings {
    char baseCharacter[64];
    char lookModifier[64];
    DWORD durationMs;
    DWORD cooldownMs;
    int key;
} g_cfg;

struct SlotState {
    bool  beastBoy = false;
    void* assist = nullptr;
    bool  hidden = false;
    ULONGLONG showAt = 0;
    ULONGLONG readyAt = 0;
};

const CharacterRow& g_char  = kCharacters[0];
const MorphRow&     g_morph = kMorphs[g_char.morph];
SlotState g_slot[kSlots];

using Fwd = int64_t (*)(int64_t, int64_t, int64_t, int64_t);
Fwd g_origTick = nullptr, g_origFightStart = nullptr, g_origAddModifiers = nullptr;

using XInputGetStateFn = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);
XInputGetStateFn g_xinput = nullptr;
bool g_wasDown = false;

void LoadSettings()
{
    IniString("BeastBoy", "BaseCharacter", g_char.baseCharacterMatch, g_cfg.baseCharacter, sizeof(g_cfg.baseCharacter));
    IniString("BeastBoy", "LookModifier", g_char.lookModifier, g_cfg.lookModifier, sizeof(g_cfg.lookModifier));
    g_cfg.durationMs = IniInt("GorillaSlam", "GorillaSlamDurationMs", (int)g_morph.durationMs);
    g_cfg.cooldownMs = IniInt("GorillaSlam", "GorillaSlamCooldownMs", (int)g_morph.cooldownMs);
    char key[8];
    char fallback[2] = { g_morph.key, 0 };
    IniString("GorillaSlam", "GorillaSlamKey", fallback, key, sizeof(key));
    g_cfg.key = (key[0] >= 'a' && key[0] <= 'z') ? key[0] - 32 : key[0];
}

bool GameHasFocus()
{
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

bool MorphPressed()
{
    bool down = false;
    if (GameHasFocus()) {
        down = (GetAsyncKeyState(g_cfg.key) & 0x8000) != 0;
        if (g_xinput) {
            for (DWORD pad = 0; pad < XUSER_MAX_COUNT && !down; ++pad) {
                XINPUT_STATE st{};
                if (g_xinput(pad, &st) == ERROR_SUCCESS && (st.Gamepad.wButtons & g_morph.padButton))
                    down = true;
            }
        }
    }
    bool pressed = down && !g_wasDown;
    g_wasDown = down;
    return pressed;
}

// The side the player controls in solo vs CPU: the first slot that is Beast Boy.
int PlayerSlot()
{
    for (int s = 0; s < kSlots; ++s)
        if (g_slot[s].beastBoy)
            return s;
    return -1;
}

void OnTick()
{
    const ULONGLONG now = GetTickCount64();
    const int s = PlayerSlot();
    if (s < 0)
        return;
    SlotState& st = g_slot[s];

    if (MorphPressed() && !st.hidden && now >= st.readyAt) {
        // The game's assist brings Grodd in on the same press; Beast Boy steps out of view.
        SetScale(GetObj(s), g_morph.hiddenScale);
        st.hidden = true;
        st.showAt = now + g_cfg.durationMs;
        st.readyAt = now + g_cfg.cooldownMs;
        Log("P%d %s: morph into %s", s + 1, g_morph.displayName, g_morph.assistCharacter);
    }
    if (st.hidden && now >= st.showAt) {
        SetScale(GetObj(s), 1.0f);
        st.hidden = false;
        Log("P%d %s: back to Beast Boy", s + 1, g_morph.displayName);
    }
}

int64_t TickHook(int64_t a, int64_t b, int64_t c, int64_t d)
{
    int64_t r = g_origTick(a, b, c, d);
    OnTick();
    return r;
}

int64_t FightStartHook(int64_t a, int64_t b, int64_t c, int64_t d)
{
    bool any = false;
    for (int s = 0; s < kSlots; ++s) {
        g_slot[s] = SlotState{};
        char name[64];
        GetCharacterName(s, name, sizeof(name));
        g_slot[s].beastBoy = strstr(name, g_cfg.baseCharacter) != nullptr;
        Log("fight start: P%d is %s%s", s + 1, name[0] ? name : "(none)",
            g_slot[s].beastBoy ? " -> Beast Boy" : "");
        if (g_slot[s].beastBoy) {
            g_slot[s].assist = CreateTagAssist(g_morph.assistCharacter, s);
            any = true;
        }
    }
    if (any)
        LoadModifierAssets();
    return g_origFightStart(a, b, c, d);
}

int64_t AddModifiersHook(int64_t a, int64_t b, int64_t c, int64_t d)
{
    int64_t r = g_origAddModifiers(a, b, c, d);
    for (int s = 0; s < kSlots; ++s) {
        if (!g_slot[s].beastBoy)
            continue;
        AttachTagAssist(g_slot[s].assist, s);
        ActivateModifier(g_cfg.lookModifier, GetObj(s));
        Log("P%d Beast Boy ready: %s look, %s on assist button", s + 1, g_cfg.lookModifier, g_morph.displayName);
    }
    return r;
}

void Start()
{
    if (!IniInt("General", "Enabled", 1))
        return;
    char logName[MAX_PATH];
    IniString("General", "LogFile", "BeastBoy.log", logName, sizeof(logName));
    LogOpen(logName);
    LoadSettings();
    Log("Beast Boy for Injustice 2 starting (base fighter %s, look %s, slam %lums / cooldown %lums)",
        g_cfg.baseCharacter, g_cfg.lookModifier, g_cfg.durationMs, g_cfg.cooldownMs);

    for (const char* dll : { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" }) {
        if (HMODULE h = LoadLibraryA(dll)) {
            g_xinput = reinterpret_cast<XInputGetStateFn>(GetProcAddress(h, "XInputGetState"));
            if (g_xinput)
                break;
        }
    }

    if (ResolveHooks() != HOOK_COUNT) {
        Log("not starting: this Injustice 2 build does not match (see MISSING above)");
        MessageBoxA(nullptr,
                    "Beast Boy mod: this version of Injustice 2 isn't supported yet, so the game will start "
                    "without Beast Boy.\n\nThe mod is built for the current Steam version.",
                    "Beast Boy for Injustice 2", MB_OK | MB_ICONWARNING);
        return;
    }
    g_origTick         = reinterpret_cast<Fwd>(RedirectCall(gHook[HOOK_ProcDispatchCall], (void*)TickHook));
    g_origFightStart   = reinterpret_cast<Fwd>(RedirectCall(gHook[HOOK_FightStartCall], (void*)FightStartHook));
    g_origAddModifiers = reinterpret_cast<Fwd>(RedirectCall(gHook[HOOK_AddModifiersCall], (void*)AddModifiersHook));
    Log("hooks installed: tick=%d fightStart=%d addModifiers=%d",
        g_origTick != nullptr, g_origFightStart != nullptr, g_origAddModifiers != nullptr);
}

} // namespace

extern "C" __declspec(dllexport) void InitializeASI()
{
    static bool started = false;
    if (!started) {
        started = true;
        Start();
    }
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID)
{
    return TRUE;
}
