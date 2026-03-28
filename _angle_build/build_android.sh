#!/bin/bash
set -e

export PATH="/home/catarina/angle_build/angle/third_party/depot_tools:/home/catarina/angle_build/angle/buildtools/linux64:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export HOME=/home/catarina

cd /home/catarina/angle_build/angle

echo "=== Building ANGLE for Android (Bionic) arm64 ==="
echo "Time: $(date)"

# Performance-optimized Android build as per ANGLE docs
GN_ARGS='target_os = "android"
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
angle_expose_non_conformant_extensions_and_versions = true'

echo "GN args:"
echo "$GN_ARGS"
echo ""

echo "=== Generating build files with GN ==="
gn gen out/Android_arm64 --args="$GN_ARGS" 2>&1
echo "=== GN exit: $? ==="

echo "=== Building ANGLE libEGL and libGLESv2 ==="
ninja -C out/Android_arm64 libEGL libGLESv2 2>&1
echo "=== Build exit: $? ==="

echo "=== Listing output ==="
ls -la out/Android_arm64/libEGL* out/Android_arm64/libGLESv2* 2>&1 || true
file out/Android_arm64/libEGL.so out/Android_arm64/libGLESv2.so 2>&1 || true

echo "=== Checking library dependencies ==="
readelf -d out/Android_arm64/libEGL.so 2>&1 | grep NEEDED || true
echo "---"
readelf -d out/Android_arm64/libGLESv2.so 2>&1 | grep NEEDED || true

echo "=== DONE at $(date) ==="

