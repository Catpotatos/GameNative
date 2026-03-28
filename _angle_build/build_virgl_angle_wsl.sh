#!/bin/bash
# ============================================================================
# Build Script: VirGL-ANGLE Hybrid — libvirglrenderer_angle.so
# ============================================================================
#
# Builds the ANGLE-backed VirGL renderer JNI library for Android arm64-v8a
# using the NDK installed at the Windows SDK path, invoked from WSL.
#
# This is a variant of libvirglrenderer.so where the host-side EGL init
# uses ANGLE's eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, ...)
# instead of eglGetDisplay(EGL_DEFAULT_DISPLAY).
#
# Component versions used in the hybrid:
#   - virglrenderer:  1.3.0 (host-side renderer, vrend core + custom JNI server/)
#   - ANGLE:          chromium/7748, commit c7ac96e (host-side EGL/GLES→Vulkan)
#   - Mesa virpipe:   23.1.9 (guest-side Gallium driver, from virgl-23.1.9.tzst)
#   - NDK:            r28.2.13676358 (Clang 19.0.1)
#
# First built: 2026-03-27
#
# Usage (from WSL):
#   bash /mnt/c/Users/Catarina/StudioProjects/GameNative-mine/_angle_build/build_virgl_angle_wsl.sh
#
# Output:
#   app/src/main/jniLibs/arm64-v8a/libvirglrenderer_angle.so

set -euo pipefail

echo "============================================================"
echo " VirGL-ANGLE Hybrid Build"
echo "============================================================"
echo ""

# ── Paths ──────────────────────────────────────────────────────────────────

# Project root (Windows path mounted in WSL)
WIN_PROJECT="/mnt/c/Users/Catarina/StudioProjects/GameNative-mine"

# NDK — use the newer NDK 28 for better arm64 codegen
ANDROID_NDK_HOME="/mnt/c/Users/Catarina/AppData/Local/Android/Sdk/ndk/28.2.13676358"
TOOLCHAIN="${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake"

# Source is on the Windows mount
SRC_DIR="${WIN_PROJECT}/app/src/main/cpp/virglrenderer_angle"

# Build in WSL native filesystem for speed (cross-mount cmake is slow)
BUILD_DIR="/tmp/virgl_angle_build_arm64"

# Output goes back to the project jniLibs
OUTPUT_DIR="${WIN_PROJECT}/app/src/main/jniLibs/arm64-v8a"

# ── Sanity checks ─────────────────────────────────────────────────────────

if [ ! -f "$TOOLCHAIN" ]; then
    echo "ERROR: Android NDK CMake toolchain not found at:"
    echo "  $TOOLCHAIN"
    exit 1
fi

if [ ! -f "${SRC_DIR}/CMakeLists.txt" ]; then
    echo "ERROR: virglrenderer_angle source not found at:"
    echo "  ${SRC_DIR}"
    exit 1
fi

echo "NDK:        $ANDROID_NDK_HOME"
echo "Toolchain:  $TOOLCHAIN"
echo "Source:     $SRC_DIR"
echo "Build:      $BUILD_DIR"
echo "Output:     $OUTPUT_DIR"
echo ""

# ── Clean & Build ──────────────────────────────────────────────────────────

echo "[1/3] Cleaning previous build..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "[2/3] Configuring with CMake..."
cmake \
    -S "$SRC_DIR" \
    -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DANDROID_STL=c++_shared \
    -DCMAKE_BUILD_TYPE=Release \
    2>&1

echo ""
echo "[3/3] Building libvirglrenderer_angle.so..."
cmake --build "$BUILD_DIR" --parallel "$(nproc)" 2>&1

# ── Install ────────────────────────────────────────────────────────────────

echo ""
echo "Installing to jniLibs..."
mkdir -p "$OUTPUT_DIR"
cp "$BUILD_DIR/libvirglrenderer_angle.so" "$OUTPUT_DIR/"

echo ""
echo "============================================================"
echo " SUCCESS: libvirglrenderer_angle.so"
echo "============================================================"
echo "Location: $OUTPUT_DIR/libvirglrenderer_angle.so"
ls -lh "$OUTPUT_DIR/libvirglrenderer_angle.so"
echo ""

# ── Verify JNI symbols ────────────────────────────────────────────────────

echo "Verifying JNI symbols..."
TOOLCHAIN_BIN=$(find "${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt" -maxdepth 1 -type d | head -1)/bin
NM="${TOOLCHAIN_BIN}/llvm-nm"
READELF="${TOOLCHAIN_BIN}/llvm-readelf"

if [ -x "$NM" ]; then
    echo ""
    echo "VirGLAngleRendererComponent JNI entry points:"
    $NM -D "$OUTPUT_DIR/libvirglrenderer_angle.so" | grep "VirGLAngle" || echo "  WARNING: No VirGLAngle symbols found!"
    echo ""
fi

if [ -x "$READELF" ]; then
    echo "Shared library dependencies:"
    $READELF -d "$OUTPUT_DIR/libvirglrenderer_angle.so" | grep NEEDED || true
    echo ""
fi

echo "============================================================"
echo " Build complete!"
echo ""
echo " The VirGL-ANGLE hybrid chain:"
echo "   Guest: Game → WineD3D → OpenGL → Mesa virpipe → socket"
echo "   Host:  virgl_server → ANGLE EGL → Vulkan → GPU"
echo ""
echo " Assets needed at runtime:"
echo "   - angle-7748.tzst (ANGLE libs) ✓ already in assets"
echo "   - virgl-23.1.9.tzst (guest Mesa) ✓ already in assets"
echo "============================================================"

