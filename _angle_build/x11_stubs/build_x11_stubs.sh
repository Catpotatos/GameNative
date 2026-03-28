#!/bin/bash
# Build minimal X11 stub library for Android/Bionic (arm64).
# This stub satisfies gl4es's dynamic libX11.so dependency at runtime.
# Deploy alongside libGL.so.1 in opt/gl4es/lib/.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CLANG="/home/catarina/angle_build/angle/third_party/llvm-build/Release+Asserts/bin/clang"
SYSROOT="/home/catarina/angle_build/angle/third_party/android_toolchain/ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot"

OUTPUT_DIR="$SCRIPT_DIR/lib"
mkdir -p "$OUTPUT_DIR"

echo "=== Building X11 stub library for Bionic arm64 ==="

$CLANG \
    --target=aarch64-linux-android28 \
    --sysroot="$SYSROOT" \
    --rtlib=compiler-rt \
    --unwindlib=none \
    -shared -fPIC \
    -O2 \
    -I"$SCRIPT_DIR/include" \
    -Wl,-soname,libX11.so \
    -o "$OUTPUT_DIR/libX11.so" \
    "$SCRIPT_DIR/x11_stub.c"

echo "=== Build complete ==="
ls -la "$OUTPUT_DIR/libX11.so"

echo ""
echo "=== Verifying bionic linkage ==="
readelf -d "$OUTPUT_DIR/libX11.so" | grep -E "NEEDED|SONAME"

echo ""
echo "=== Checking architecture ==="
file "$OUTPUT_DIR/libX11.so"

echo ""
echo "=== Verifying exported symbols ==="
echo "Core X11 functions:"
nm -D "$OUTPUT_DIR/libX11.so" | grep -cE " T (XOpenDisplay|XCreateWindow|XCreateSimpleWindow|XGetWindowAttributes|XGetVisualInfo|XMapWindow)" || echo "  WARNING: missing core symbols"
echo "New stubs (keyboard/extensions/selection):"
nm -D "$OUTPUT_DIR/libX11.so" | grep -cE " T (XKeysymToKeycode|XListExtensions|XQueryExtension|XSetSelectionOwner|XFetchName|XGetWMHints)" || echo "  WARNING: missing new stub symbols"
echo "All exported X* functions:"
nm -D "$OUTPUT_DIR/libX11.so" | grep " T X" | wc -l

echo ""
echo "Output: $OUTPUT_DIR/libX11.so"
echo "Copy this into opt/gl4es/lib/ alongside libGL.so.1 in the gl4es tarball."
echo "=== Done ==="

