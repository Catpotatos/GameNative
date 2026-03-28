#!/bin/bash
set -e
export PATH="/usr/bin:/usr/local/bin:/usr/sbin:/bin:/sbin:/home/catarina/angle_build/angle/third_party/depot_tools:$PATH"

cd /home/catarina/angle_build/angle

echo "=== Rebuilding ANGLE with ES3 fix ==="
echo "Patch verification:"
sed -n '5089,5101p' src/libANGLE/renderer/vulkan/vk_renderer.cpp

echo ""
echo "=== Running GN gen ==="
gn gen out/Android_arm64_7748 2>&1 | tail -5

echo ""
echo "=== Starting ninja build ==="
ninja -C out/Android_arm64_7748 libEGL libGLESv2 2>&1
BUILD_EXIT=$?
echo "=== Build exit: $BUILD_EXIT ==="

if [ $BUILD_EXIT -ne 0 ]; then
    echo "BUILD FAILED!"
    exit 1
fi

echo ""
echo "=== Verifying ==="
ls -la out/Android_arm64_7748/libEGL_angle.so out/Android_arm64_7748/libGLESv2_angle.so
file out/Android_arm64_7748/libGLESv2_angle.so

echo ""
echo "=== Packaging ==="
PKG_DIR="/home/catarina/angle_build/package_7748_es3fix"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/opt/angle/lib"
cp out/Android_arm64_7748/libEGL_angle.so "$PKG_DIR/opt/angle/lib/libEGL.so"
cp out/Android_arm64_7748/libGLESv2_angle.so "$PKG_DIR/opt/angle/lib/libGLESv2.so"
patchelf --set-soname libEGL.so "$PKG_DIR/opt/angle/lib/libEGL.so"
patchelf --set-soname libGLESv2.so "$PKG_DIR/opt/angle/lib/libGLESv2.so"

echo "Packaged:"
ls -lh "$PKG_DIR/opt/angle/lib/"
readelf -d "$PKG_DIR/opt/angle/lib/libEGL.so" | grep SONAME
readelf -d "$PKG_DIR/opt/angle/lib/libGLESv2.so" | grep SONAME

echo ""
echo "=== Creating archive ==="
cd "$PKG_DIR"
tar cf - opt/angle | zstd -19 -o /home/catarina/angle_build/angle-7748-es3fix.tzst --force
ls -lh /home/catarina/angle_build/angle-7748-es3fix.tzst

echo ""
echo "=== Deploying to assets ==="
WIN_ASSETS="/mnt/c/Users/Catarina/StudioProjects/GameNative-mine/app/src/main/assets/graphics_driver"
cp /home/catarina/angle_build/angle-7748-es3fix.tzst "$WIN_ASSETS/angle-7748.tzst"
ls -lh "$WIN_ASSETS/angle-7748.tzst"

echo ""
echo "=========================================="
echo "=== ANGLE ES3 FIX BUILD COMPLETE      ==="
echo "=========================================="

