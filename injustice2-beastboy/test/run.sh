#!/bin/sh
# Builds the harness and runs it under Wine against the built plugin.
set -e
cd "$(dirname "$0")/.."
./build.sh >/dev/null
mkdir -p build/test
x86_64-w64-mingw32-g++ -std=c++17 -O1 -static -o build/test/harness.exe test/harness.cpp src/core.cpp -Isrc -luser32
cp build/zBeastBoy.asi build/test/
cd build/test
WINEDEBUG=-all timeout 120 wine harness.exe zBeastBoy.asi
echo "--- BeastBoy.log ---"
cat BeastBoy.log
