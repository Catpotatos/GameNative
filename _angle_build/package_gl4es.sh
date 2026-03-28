#!/bin/bash
set -e
OUTPUT=/home/catarina/gl4es_build/gl4es/lib/libGL.so.1
BUILD_DIR=/home/catarina/gl4es_build/build_bionic
WIN_ASSETS="/mnt/c/Users/Catarina/StudioProjects/GameNative-mine/app/src/main/assets/graphics_driver"

echo "=== Verifying build ==="
ls -la "$OUTPUT"
file "$OUTPUT"
echo "--- NEEDED/SONAME ---"
readelf -d "$OUTPUT" | grep -E "NEEDED|SONAME"
echo "--- GLX symbols ---"
GLX_COUNT=$(readelf -Ws "$OUTPUT" | grep -ci "glX" || true)
echo "Found $GLX_COUNT glX* symbols"
readelf -Ws "$OUTPUT" | grep -iE "glXChooseVisual|glXCreateContext|glXMakeCurrent|glXSwapBuffers" | head -10
echo "--- X11 stub symbols ---"
X11_SYM_COUNT=$(readelf -Ws "$OUTPUT" | grep -cE "XOpenDisplay|XGetVisualInfo|XCreateColormap" || true)
echo "Found $X11_SYM_COUNT X11 stub symbols statically linked"

echo ""
echo "=== Packaging ==="
rm -rf "$BUILD_DIR/output"
mkdir -p "$BUILD_DIR/output/opt/gl4es/lib"
cp "$OUTPUT" "$BUILD_DIR/output/opt/gl4es/lib/libGL.so.1"
cd "$BUILD_DIR/output"
tar cf - opt/gl4es | zstd -19 -o "$BUILD_DIR/gl4es-bionic-1.1.8.tzst"
ls -la "$BUILD_DIR/gl4es-bionic-1.1.8.tzst"

echo ""
echo "=== Copying to Android assets ==="
mkdir -p "$WIN_ASSETS"
cp "$BUILD_DIR/gl4es-bionic-1.1.8.tzst" "$WIN_ASSETS/gl4es-bionic-1.1.8.tzst"
ls -la "$WIN_ASSETS/gl4es-bionic-1.1.8.tzst"

echo ""
echo "=========================================="
echo "=== GL4ES BUILD AND DEPLOY COMPLETE ==="
echo "=========================================="

