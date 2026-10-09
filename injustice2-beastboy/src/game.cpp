#include "game.h"
#include "core.h"
#include <cstring>
#include <cstdlib>

namespace bb {

uintptr_t gHook[HOOK_COUNT] = {};

int ResolveHooks()
{
    int found = 0;
    for (int i = 0; i < HOOK_COUNT; ++i) {
        const HookRow& row = kHooks[i];
        uintptr_t at = FindPattern(row.pattern);
        if (at) {
            at += row.offset;
            if (row.kind == KIND_RIPREL) {
                int32_t disp;
                memcpy(&disp, reinterpret_cast<void*>(at), 4);
                at = at + 4 + (intptr_t)disp;
            }
            ++found;
        }
        gHook[i] = at;
        Log("hook %-26s %s (%s)", row.id, at ? "found" : "MISSING", row.purpose);
    }
    return found;
}

template <typename Fn>
static Fn Call(HookId id) { return reinterpret_cast<Fn>(gHook[id]); }

static void* GameInfo()
{
    if (!gHook[HOOK_GameInfoPtr])
        return nullptr;
    return *reinterpret_cast<void**>(gHook[HOOK_GameInfoPtr]);
}

PlayerInfo* GetInfo(int player)
{
    void* gi = GameInfo();
    if (!gi)
        return nullptr;
    return Call<PlayerInfo* (*)(void*, int)>(HOOK_GameInfo_GetInfo)(gi, player);
}

Character* GetObj(int player)
{
    PlayerInfo* info = GetInfo(player);
    if (!info)
        return nullptr;
    return Call<Character* (*)(PlayerInfo*)>(HOOK_PlayerInfo_GetObj)(info);
}

void GetCharacterName(int player, char* out, int outSize)
{
    out[0] = 0;
    auto info = reinterpret_cast<uint8_t*>(GetInfo(player));
    if (!info)
        return;
    // PlayerInfo + 192 holds the character definition, whose first field is its name.
    const char* name = *reinterpret_cast<const char**>(info + 192);
    if (!name)
        return;
    int i = 0;
    for (; i < outSize - 1 && name[i]; ++i)
        out[i] = name[i];
    out[i] = 0;
}

void SetScale(Character* chr, float s)
{
    if (!chr)
        return;
    FVector v{ s, s, s };
    Call<void (*)(Character*, FVector*)>(HOOK_Character_SetScale)(chr, &v);
}

void ActivateModifier(const char* name, Character* chr)
{
    if (!chr)
        return;
    alignas(16) uint8_t modifier[128] = {};
    Call<void (*)(void*, const char*)>(HOOK_Modifier_Construct)(modifier, name);
    Call<void (*)(void*, Character*)>(HOOK_Modifier_Activate)(modifier, chr);
}

void* CreateTagAssist(const char* character, int player)
{
    PlayerInfo* info = GetInfo(player);
    if (!info)
        return nullptr;
    // The engine keeps a pointer to this for the whole fight, so it is never freed.
    void* assist = _aligned_malloc(128, 16);
    memset(assist, 0, 128);
    Call<void (*)(void*, const char*)>(HOOK_TagAssist_Construct)(assist, character);
    Call<void (*)(void*, PlayerInfo*)>(HOOK_TagAssist_ActivateInfo)(assist, info);
    return assist;
}

void AttachTagAssist(void* assist, int player)
{
    Character* chr = GetObj(player);
    if (!assist || !chr)
        return;
    Call<void (*)(void*, Character*)>(HOOK_TagAssist_ActivateObject)(assist, chr);
}

void LoadModifierAssets()
{
    if (void* gi = GameInfo())
        Call<void (*)(void*)>(HOOK_LoadModifierAssets)(gi);
}

} // namespace bb
