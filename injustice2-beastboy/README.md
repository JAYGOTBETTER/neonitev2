# Beast Boy for Injustice 2

A native plugin for Injustice 2 (Steam, 64-bit, NetherRealm's Unreal Engine 3 build).
Whoever picks **Cheetah** fights as **Beast Boy**: he glows with the game's own green
Lantern light, and his assist button (right stick click / `P`) morphs him into
**Gorilla Grodd** for a slam. Grodd's model comes from the player's own install, through
the game's tag-assist system. Solo vs CPU.

## What a player gets
- Cheetah's slot plays as Beast Boy, with a green look (`GreenLanternsLight` modifier).
- **Gorilla Slam**: the assist button brings Grodd in while Beast Boy steps out of view, then
  Beast Boy reappears (1.5 s, 6 s recharge; both set in `BeastBoy.ini`).
- Single player: Versus vs CPU, Training, Multiverse. Not for online play.

## How it is built
- `sheets/*.json` are the design, and the source of truth: characters, morphs, game hooks, config.
  `python3 tools/sheets.py preflight` lists unfilled cells, broken references and cells still
  awaiting in-game verification. `generate` writes `src/generated/sheets.h` and `BeastBoy.ini`.
- `src/` is the plugin: pattern scan → three call-site redirects (frame tick, fight start,
  fighters spawned) → the engine calls in `game.cpp`. If any pattern is missing it logs which one,
  tells the player the game version isn't supported, and stays off.
- Loaded by Ultimate ASI Loader (`dinput8.dll`, MIT, pinned v9.7.4, fetched by `tools/fetch-loader.sh`).
- The game addresses are the byte patterns from ermaccer's I2Hook research (current Steam build).

```
./build.sh        # MinGW-w64 → build/zBeastBoy.asi + BeastBoy.ini
./test/run.sh     # Wine harness: pattern scan, call redirect, plugin load
./package.sh      # dist/BeastBoy-Injustice2-<version>.zip (root = Injustice 2 install folder)
```

## Status
v0.1.0 builds and passes the harness. It has **not run inside Injustice 2 yet**; see MODLOG.md
for what still needs checking in the game.
