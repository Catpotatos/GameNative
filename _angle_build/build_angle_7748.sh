#!/bin/bash
# Build ANGLE chromium/7748 for Android/Bionic (arm64) with Vulkan backend.
# Produces libEGL.so and libGLESv2.so for use in GameNative Bionic container.
set -e

export PATH="/usr/bin:/usr/local/bin:/usr/sbin:/bin:/sbin:/home/catarina/angle_build/angle/third_party/depot_tools:$PATH"

ANGLE_SRC="/home/catarina/angle_build/angle"
OUT_DIR="out/Android_arm64_7748"
PKG_DIR="/home/catarina/angle_build/package_7748"
WIN_ASSETS="/mnt/c/Users/Catarina/StudioProjects/GameNative-mine/app/src/main/assets/graphics_driver"
LOG="/home/catarina/angle_build/build_7748.log"

echo "=== ANGLE 7748 Build Script ===" | tee "$LOG"
echo "Started: $(date)" | tee -a "$LOG"

cd "$ANGLE_SRC"
echo "Branch: $(git branch --show-current)" | tee -a "$LOG"
echo "Commit: $(git log --oneline -1)" | tee -a "$LOG"

echo "" | tee -a "$LOG"
echo "=== Step 1: gclient sync ===" | tee -a "$LOG"
cd /home/catarina/angle_build
gclient sync --shallow --no-history -D 2>&1 | tee -a "$LOG"
echo "=== gclient sync exit: $? ===" | tee -a "$LOG"

cd "$ANGLE_SRC"

echo "" | tee -a "$LOG"
echo "=== Step 2: Generating build files with GN ===" | tee -a "$LOG"

mkdir -p "$OUT_DIR"
cat > "$OUT_DIR/args.gn" << 'EOF'
target_os = "android"
target_cpu = "arm64"
arm_control_flow_integrity = "none"

is_component_build = false
is_debug = false

# Performance
use_thin_lto = true
thin_lto_enable_optimizations = true

# Vulkan backend (core requirement)
angle_enable_vulkan = true

# Disable backends not needed on Android
angle_enable_gl = false
angle_enable_null = false
angle_enable_metal = false
angle_enable_d3d9 = false
angle_enable_d3d11 = false
angle_enable_wgpu = false
angle_enable_swiftshader = false

# Game compatibility - critical for Wine/WineD3D games
angle_expose_non_conformant_extensions_and_versions = true

# Reduce build size - disable unnecessary features
angle_build_all = false
build_angle_deqp_tests = false
angle_build_tests = false
enable_rust = false
treat_warnings_as_errors = false

# Disable validation for production
angle_enable_vulkan_validation_layers = false
angle_assert_always_on = false
EOF

echo "args.gn:" | tee -a "$LOG"
cat "$OUT_DIR/args.gn" | tee -a "$LOG"

gn gen "$OUT_DIR" 2>&1 | tee -a "$LOG"
echo "=== GN exit: $? ===" | tee -a "$LOG"

echo "" | tee -a "$LOG"
echo "=== Step 3: Building ANGLE ===" | tee -a "$LOG"
ninja -C "$OUT_DIR" libEGL libGLESv2 2>&1 | tee -a "$LOG"
echo "=== Build exit: $? ===" | tee -a "$LOG"

echo "" | tee -a "$LOG"
echo "=== Step 4: Verifying build ===" | tee -a "$LOG"
ls -la "$OUT_DIR"/libEGL_angle.so "$OUT_DIR"/libGLESv2_angle.so 2>&1 | tee -a "$LOG"
file "$OUT_DIR"/libEGL_angle.so "$OUT_DIR"/libGLESv2_angle.so 2>&1 | tee -a "$LOG"
echo "" | tee -a "$LOG"
echo "Dependencies:" | tee -a "$LOG"
readelf -d "$OUT_DIR"/libEGL_angle.so | grep -E "NEEDED|SONAME" | tee -a "$LOG"
echo "---" | tee -a "$LOG"
readelf -d "$OUT_DIR"/libGLESv2_angle.so | grep -E "NEEDED|SONAME" | tee -a "$LOG"

echo "" | tee -a "$LOG"
echo "=== Step 5: Packaging ===" | tee -a "$LOG"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/opt/angle/lib"

# Copy and rename (_angle suffix -> standard names)
cp "$OUT_DIR/libEGL_angle.so" "$PKG_DIR/opt/angle/lib/libEGL.so"
cp "$OUT_DIR/libGLESv2_angle.so" "$PKG_DIR/opt/angle/lib/libGLESv2.so"

# Patch SONAMEs so dlopen("libEGL.so") resolves correctly
patchelf --set-soname libEGL.so "$PKG_DIR/opt/angle/lib/libEGL.so"
patchelf --set-soname libGLESv2.so "$PKG_DIR/opt/angle/lib/libGLESv2.so"
echo "SONAMEs patched." | tee -a "$LOG"

echo "" | tee -a "$LOG"
echo "Packaged libraries:" | tee -a "$LOG"
ls -lh "$PKG_DIR/opt/angle/lib/" | tee -a "$LOG"
readelf -d "$PKG_DIR/opt/angle/lib/libEGL.so" | grep SONAME | tee -a "$LOG"
readelf -d "$PKG_DIR/opt/angle/lib/libGLESv2.so" | grep SONAME | tee -a "$LOG"

echo "" | tee -a "$LOG"
echo "=== Step 6: Creating archive ===" | tee -a "$LOG"
cd "$PKG_DIR"
tar cf - opt/angle | zstd -19 -o /home/catarina/angle_build/angle-7748-android.tzst --force
echo "Archive created:" | tee -a "$LOG"
ls -lh /home/catarina/angle_build/angle-7748-android.tzst | tee -a "$LOG"

echo "" | tee -a "$LOG"
echo "=== Step 7: Copying to Android assets ===" | tee -a "$LOG"
mkdir -p "$WIN_ASSETS"
cp /home/catarina/angle_build/angle-7748-android.tzst "$WIN_ASSETS/angle-7748.tzst"
echo "Copied to: $WIN_ASSETS/angle-7748.tzst" | tee -a "$LOG"
ls -lh "$WIN_ASSETS/angle-7748.tzst" | tee -a "$LOG"

echo "" | tee -a "$LOG"
echo "==========================================" | tee -a "$LOG"
echo "=== ANGLE 7748 BUILD AND DEPLOY COMPLETE ===" | tee -a "$LOG"
echo "Finished: $(date)" | tee -a "$LOG"
echo "==========================================" | tee -a "$LOG"

