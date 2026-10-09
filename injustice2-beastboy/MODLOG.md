# MODLOG: Beast Boy for Injustice 2

## Idea (from the interview)
- Beast Boy mod inside Injustice 2. Shapeshifter: specials morph him briefly into an animal.
- Takes **Cheetah's** slot, recolored green. First version: **Gorilla Slam** only. **Solo vs CPU**.

## Recon
- Injustice 2: Steam app 627270, 64-bit, NetherRealm-modified Unreal Engine 3. Exe in `Binaries\Retail`.
- Established route: ASI plugin via Ultimate ASI Loader (`dinput8.dll`), as I2Hook (ermaccer) does.
  I2Hook has no license file, so none of its code or binaries are shipped. Only its published byte
  patterns (facts about the game exe) are reused, with credit.
- I2Hook shows the engine has: tag-assist modifiers (`TagAssistModifier("GorillaGrodd")`),
  Multiverse modifiers by name (`GreenLanternsLight`), visual-only `SetScale`, per-player
  `GetInfo/GetObj`, and the character definition name at `PlayerInfo+192`.
- Assist input: right stick click on a pad, `P` on keyboard (Steam discussion, app 627270).

## Route
Own standalone ASI (`zBeastBoy.asi`, named to load after I2Hook if both are present; each call
redirect calls whatever the call site pointed at before, so they chain).
- Fight start: if a side is Cheetah, queue a Grodd tag assist for it and load modifier assets.
- Fighters spawned: attach the assist and apply `GreenLanternsLight` to Beast Boy.
- Every frame: on an assist-button press (edge, with cooldown), scale Beast Boy to 0.01 for the
  morph duration while the game's Grodd assist attacks, then scale back to 1.

## Verified (cloud session, no game)
- Sheets preflight: 0 errors.
- Wine harness: FindPattern with wildcards, E8 call redirect through a near stub with argument
  forwarding, plugin loads, exports InitializeASI, and refuses to hook a non-Injustice exe.

## Not yet verified (needs Injustice 2 running on the player's PC)
1. All 13 patterns found in the current Steam exe (BeastBoy.log lists each).
2. The green look `GreenLanternsLight` gives Cheetah.
3. The Grodd assist appears and is called by RS / P in Versus vs CPU.
4. durationMs (1500) matches Grodd's time on screen; cooldownMs (6000) matches the game's
   assist cooldown, so Beast Boy never disappears without the gorilla.
5. Screenshot/clip of the morph for the Melty listing.

## Blockers
- melty.gg is denied by this cloud environment's network policy: no game_info, search_mashups,
  recipe checks or upload yet.
- This cloud session cannot run Injustice 2; testing needs a Claude session on the player's PC.
