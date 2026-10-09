// Injustice 2 engine calls, resolved from sheets/hooks.json patterns.
#pragma once
#include <cstdint>
#include "generated/sheets.h"

namespace bb {

struct FVector { float X, Y, Z; };

// Opaque engine objects.
struct PlayerInfo;
struct Character;

extern uintptr_t gHook[HOOK_COUNT];

// Scans every hook row; returns how many were found and logs each one.
int  ResolveHooks();

PlayerInfo* GetInfo(int player);
Character*  GetObj(int player);
// Character definition name for a player (e.g. "CHAR_Cheetah_MSTR"), copied into out.
void        GetCharacterName(int player, char* out, int outSize);

void SetScale(Character* chr, float s);
void ActivateModifier(const char* name, Character* chr);

// The game's tag-assist modifier: a second fighter (loaded from the player's own
// Injustice 2 install) that jumps in when the assist button is pressed.
void* CreateTagAssist(const char* character, int player);
void  AttachTagAssist(void* assist, int player);
void  LoadModifierAssets();

} // namespace bb
