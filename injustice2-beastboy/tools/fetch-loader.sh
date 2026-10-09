#!/bin/sh
# Downloads the pinned Ultimate ASI Loader (MIT) into third_party/ and checks its hash.
set -e
cd "$(dirname "$0")/../third_party/ultimate-asi-loader"
[ -f dinput8.dll ] && exit 0
URL=https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/v9.7.4/Ultimate-ASI-Loader_x64.zip
SHA=8272d83b2692662098746f2d0ad0e2d85f3c8358ab1d63f75fbe835c2c8135fd
TMP=$(mktemp -d)
curl -sSfL -o "$TMP/ual.zip" "$URL"
echo "$SHA  $TMP/ual.zip" | sha256sum -c -
unzip -q -o "$TMP/ual.zip" dinput8.dll -d .
rm -rf "$TMP"
