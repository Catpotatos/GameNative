#!/bin/bash
# Build ANGLE for Android/Bionic (arm64) with Vulkan backend.
# Produces libEGL.so and libGLESv2.so for use in GameNative Bionic container.
#
# Run in WSL Ubuntu:  bash /home/catarina/angle_build/build_angle.sh
set -e

ANGLE_SRC="/home/catarina/angle_build/angle"
OUT_DIR="out/Android_arm64"
PKG_DIR="/home/catarina/angle_build/package_android"
WIN_ASSETS="/mnt/c/Users/Catarina/StudioProjects/GameNative-mine/app/src/main/assets/graphics_driver"

cd "$ANGLE_SRC"
export PATH=$PWD/buildtools/linux64:$PATH

echo "=== Step 1: Generating build files with GN ==="

# IMPORTANT: target_os MUST be "android" (not "linux")
# - "android" → Bionic linker, VK_KHR_android_surface WSI → works in container
# - "linux"   → glibc linker, VK_KHR_xcb_surface WSI    → BROKEN in container
mkdir -p "$OUT_DIR"
cat > "$OUT_DIR/args.gn" << 'EOF'
target_os = "android"
target_cpu = "arm64"
arm_control_flow_integrity = "none"
is_debug = false
is_component_build = false
angle_enable_vulkan = true
angle_enable_gl = false
angle_enable_null = false
angle_enable_metal = false
angle_enable_d3d9 = false
angle_enable_d3d11 = false
angle_enable_wgpu = false
angle_enable_swiftshader = false
angle_build_all = false
enable_rust = false
treat_warnings_as_errors = false
angle_expose_non_conformant_extensions_and_versions = true
EOF

cat "$OUT_DIR/args.gn"
gn gen "$OUT_DIR" 2>&1
echo "=== GN exit: $? ==="

echo ""
echo "=== Step 2: Building ANGLE ==="
ninja -C "$OUT_DIR" libEGL libGLESv2 2>&1
echo "=== Build exit: $? ==="

echo ""
echo "=== Step 3: Verifying build ==="
ls -la "$OUT_DIR"/libEGL_angle.so "$OUT_DIR"/libGLESv2_angle.so
file "$OUT_DIR"/libEGL_angle.so "$OUT_DIR"/libGLESv2_angle.so
echo ""
echo "Dependencies:"
readelf -d "$OUT_DIR"/libEGL_angle.so | grep -E "NEEDED|SONAME"
echo "---"
readelf -d "$OUT_DIR"/libGLESv2_angle.so | grep -E "NEEDED|SONAME"

echo ""
echo "=== Step 4: Packaging ==="
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/opt/angle/lib"

# Copy and rename (_angle suffix → standard names)
cp "$OUT_DIR/libEGL_angle.so" "$PKG_DIR/opt/angle/lib/libEGL.so"
cp "$OUT_DIR/libGLESv2_angle.so" "$PKG_DIR/opt/angle/lib/libGLESv2.so"

# Patch SONAMEs so dlopen("libEGL.so") resolves correctly
if command -v patchelf &>/dev/null; then
    patchelf --set-soname libEGL.so "$PKG_DIR/opt/angle/lib/libEGL.so"
    patchelf --set-soname libGLESv2.so "$PKG_DIR/opt/angle/lib/libGLESv2.so"
    echo "SONAMEs patched."
else
    echo "WARNING: patchelf not found — install with: sudo apt install patchelf"
    echo "SONAMEs NOT patched. dlopen may fail at runtime."
fi

echo ""
echo "Packaged libraries:"
ls -lh "$PKG_DIR/opt/angle/lib/"
readelf -d "$PKG_DIR/opt/angle/lib/libEGL.so" | grep SONAME
readelf -d "$PKG_DIR/opt/angle/lib/libGLESv2.so" | grep SONAME

echo ""
echo "=== Step 5: Creating archive ==="
cd "$PKG_DIR"
tar cf - opt/angle | zstd -19 -o /home/catarina/angle_build/angle-7736-android.tzst
echo "Archive created:"
ls -lh /home/catarina/angle_build/angle-7736-android.tzst

echo ""
echo "=== Step 6: Copying to Android assets ==="
mkdir -p "$WIN_ASSETS"
cp /home/catarina/angle_build/angle-7736-android.tzst "$WIN_ASSETS/angle-7736.tzst"
echo "Copied to: $WIN_ASSETS/angle-7736.tzst"
ls -lh "$WIN_ASSETS/angle-7736.tzst"

echo ""
echo "=========================================="
echo "=== ANGLE BUILD AND DEPLOY COMPLETE ==="
echo "=========================================="
