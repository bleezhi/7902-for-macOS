#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="${VERSION:-0.1.0-alpha.1}"
PRODUCT="MT7902Wlan"
OUT="$ROOT/build/$PRODUCT.kext"
BIN="$OUT/Contents/MacOS/$PRODUCT"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "error: this KEXT must be built on macOS with Apple's kernel headers." >&2
    exit 1
fi

SDK="$(xcrun --sdk macosx --show-sdk-path)"
KERNEL_HEADERS="${KDK_KERNEL_HEADERS:-}"

if [[ -z "$KERNEL_HEADERS" ]]; then
    for candidate in \
        /Library/Developer/KDKs/*/System/Library/Frameworks/Kernel.framework/Headers \
        "$SDK/System/Library/Frameworks/Kernel.framework/Headers"; do
        if [[ -f "$candidate/IOKit/IOService.h" ]]; then
            KERNEL_HEADERS="$candidate"
            break
        fi
    done
fi

if [[ -z "$KERNEL_HEADERS" || ! -f "$KERNEL_HEADERS/IOKit/IOService.h" ]]; then
    cat >&2 <<'MSG'
error: Apple kernel headers were not found.
Install the Kernel Debug Kit (KDK) matching the macOS build used to compile
the extension, then set KDK_KERNEL_HEADERS to:
  /Library/Developer/KDKs/<matching-KDK>/System/Library/Frameworks/Kernel.framework/Headers
The ordinary Command Line Tools SDK is not guaranteed to contain KEXT headers.
MSG
    exit 2
fi

BUNDLE_VERSION="$(printf '%s' "$VERSION" | sed -E 's/[^0-9.].*$//')"
if [[ -z "$BUNDLE_VERSION" ]]; then BUNDLE_VERSION="0.1.0"; fi
mkdir -p "$(dirname "$BIN")"
rm -f "$BIN"

CLANG="$(xcrun --find clang++)"
"$CLANG" \
    -arch x86_64 \
    -DKERNEL \
    -mkernel \
    -fapple-kext \
    -fno-exceptions \
    -fno-rtti \
    -fno-common \
    -fno-builtin \
    -nostdlib \
    -isystem "$KERNEL_HEADERS" \
    -I"$ROOT/MT7902Wlan" \
    -Wall -Wextra -Werror -Wno-deprecated-declarations \
    -c "$ROOT/MT7902Wlan/MT7902Wlan.cpp" \
    -o "$ROOT/build/MT7902Wlan.o"

LD="$(xcrun --find ld)"
"$LD" -arch x86_64 -kext -r \
    -o "$BIN" "$ROOT/build/MT7902Wlan.o"

mkdir -p "$OUT/Contents"
cp "$ROOT/MT7902Wlan/Info.plist" "$OUT/Contents/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $BUNDLE_VERSION" "$OUT/Contents/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $BUNDLE_VERSION" "$OUT/Contents/Info.plist"

echo "Built $OUT"
