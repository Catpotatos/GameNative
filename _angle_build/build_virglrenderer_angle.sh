#!/bin/bash
# ============================================================================
# Build Script: libvirglrenderer_angle.so
# ============================================================================
#
# Builds the ANGLE-backed VirGL renderer JNI library for Android arm64-v8a.
# This is a variant of libvirglrenderer.so that uses ANGLE's EGL/GLES→Vulkan
# instead of the system EGL/GLES driver as the host-side rendering backend.
#
# Component versions used in the hybrid:
#   - virglrenderer:  1.3.0 (host-side renderer, vrend core + custom JNI server/)
#   - ANGLE:          chromium/7748, commit c7ac96e (host-side EGL/GLES→Vulkan)
#   - Mesa virpipe:   23.1.9 (guest-side Gallium driver, from virgl-23.1.9.tzst)
#
# Prerequisites:
#   - Android NDK (r25c or later)
#   - CMake 3.22.1+
#   - The virglrenderer_angle source tree at app/src/main/cpp/virglrenderer_angle/
#
# Usage:
#   export ANDROID_NDK_HOME=/path/to/ndk
#   bash _angle_build/build_virglrenderer_angle.sh
#
# Output:
#   app/src/main/jniLibs/arm64-v8a/libvirglrenderer_angle.so

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SRC_DIR="$PROJECT_ROOT/app/src/main/cpp/virglrenderer_angle"
BUILD_DIR="$SRC_DIR/build_arm64"
OUTPUT_DIR="$PROJECT_ROOT/app/src/main/jniLibs/arm64-v8a"

# Auto-detect NDK
if [ -z "${ANDROID_NDK_HOME:-}" ]; then
    # Try common locations
    for candidate in \
        "$HOME/Android/Sdk/ndk/"*/  \
        "$PROJECT_ROOT/local.properties"; do
        if [ -d "$candidate" ]; then
            ANDROID_NDK_HOME="$candidate"
            break
        fi
    done
    # Try reading from local.properties
    if [ -z "${ANDROID_NDK_HOME:-}" ] && [ -f "$PROJECT_ROOT/local.properties" ]; then
        NDK_DIR=$(grep 'ndk.dir' "$PROJECT_ROOT/local.properties" | cut -d= -f2 | tr -d '[:space:]')
        if [ -d "$NDK_DIR" ]; then
            ANDROID_NDK_HOME="$NDK_DIR"
        fi
    fi
fi

if [ -z "${ANDROID_NDK_HOME:-}" ] || [ ! -d "$ANDROID_NDK_HOME" ]; then
    echo "ERROR: ANDROID_NDK_HOME not set or not found."
    echo "Set it to your NDK installation directory, e.g.:"
    echo "  export ANDROID_NDK_HOME=\$HOME/Android/Sdk/ndk/25.2.9519653"
    exit 1
fi

TOOLCHAIN="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake"
if [ ! -f "$TOOLCHAIN" ]; then
    echo "ERROR: Android CMake toolchain not found at: $TOOLCHAIN"
    exit 1
fi

echo "============================================================"
echo "Building libvirglrenderer_angle.so"
echo "============================================================"
echo "NDK:        $ANDROID_NDK_HOME"
echo "Source:     $SRC_DIR"
echo "Build:      $BUILD_DIR"
echo "Output:     $OUTPUT_DIR"
echo ""

# Clean previous build
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Configure with CMake
cmake -S "$SRC_DIR" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DANDROID_STL=c++_shared \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build "$BUILD_DIR" --parallel "$(nproc 2>/dev/null || echo 4)"

# Install to jniLibs
mkdir -p "$OUTPUT_DIR"
cp "$BUILD_DIR/libvirglrenderer_angle.so" "$OUTPUT_DIR/"

echo ""
echo "============================================================"
echo "SUCCESS: libvirglrenderer_angle.so built and installed"
echo "Output: $OUTPUT_DIR/libvirglrenderer_angle.so"
echo "Size:   $(du -h "$OUTPUT_DIR/libvirglrenderer_angle.so" | cut -f1)"
echo "============================================================"

# Verify JNI symbols
echo ""
echo "Verifying JNI symbols:"
TOOLCHAIN_DIR=$(find "$ANDROID_NDK_HOME/toolchains/llvm/prebuilt" -maxdepth 1 -type d | head -1)
NM="$TOOLCHAIN_DIR/bin/llvm-nm"
if [ -x "$NM" ]; then
    echo "  VirGLAngleRendererComponent symbols:"
    $NM -D "$OUTPUT_DIR/libvirglrenderer_angle.so" | grep "VirGLAngle" || echo "  (none found — check build!)"
else
    echo "  (llvm-nm not found, skipping symbol check)"
fi

