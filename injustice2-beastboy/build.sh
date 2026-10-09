#!/bin/sh
# Builds dist/zBeastBoy.asi (64-bit Windows DLL) with MinGW-w64, from the sheets.
set -e
cd "$(dirname "$0")"
python3 tools/sheets.py generate
mkdir -p build
x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall -Wextra -Wno-cast-function-type -shared \
  -static -static-libgcc -static-libstdc++ \
  -o build/zBeastBoy.asi src/beastboy.cpp src/game.cpp src/core.cpp -Isrc -luser32
cp src/generated/BeastBoy.ini build/BeastBoy.ini
echo "built build/zBeastBoy.asi"
