#!/bin/sh
# Builds and zips the release. Zip root = the Injustice 2 install folder
# (e.g. ...\steamapps\common\Injustice2), so everything lands in Binaries\Retail.
set -e
cd "$(dirname "$0")"
VERSION=$(cat VERSION)
./build.sh
./tools/fetch-loader.sh
STAGE=build/stage
rm -rf "$STAGE" && mkdir -p "$STAGE/Binaries/Retail" "$STAGE/Binaries/Retail/BeastBoy-licenses"
cp build/zBeastBoy.asi build/BeastBoy.ini "$STAGE/Binaries/Retail/"
cp third_party/ultimate-asi-loader/dinput8.dll "$STAGE/Binaries/Retail/"
cp third_party/ultimate-asi-loader/LICENSE "$STAGE/Binaries/Retail/BeastBoy-licenses/Ultimate-ASI-Loader-LICENSE.txt"
cp PLAYER-README.txt "$STAGE/Binaries/Retail/BeastBoy-README.txt"
mkdir -p dist
OUT="dist/BeastBoy-Injustice2-$VERSION.zip"
rm -f "$OUT"
(cd "$STAGE" && TZ=UTC find . -exec touch -d 2026-01-01T00:00:00 {} + && zip -X -q -r "../../$OUT" Binaries)
echo "$OUT $(stat -c %s "$OUT") bytes sha256 $(sha256sum "$OUT" | cut -d' ' -f1)"
unzip -l "$OUT"
