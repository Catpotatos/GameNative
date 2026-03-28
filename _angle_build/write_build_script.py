#!/usr/bin/env python3
"""Write the ANGLE build script to WSL filesystem and run it."""
import subprocess
import sys

script_content = '''#!/bin/bash
set -ex
cd /home/catarina/angle_build/angle
export PATH=$PWD/buildtools/linux64:$PATH

rm -rf out/Release_arm64
mkdir -p out/Release_arm64

gn gen out/Release_arm64 --args='target_os="linux" target_cpu="arm64" is_debug=false is_component_build=false angle_enable_vulkan=true angle_enable_gl=false angle_enable_gl_desktop_frontend=false angle_enable_null=false angle_enable_metal=false angle_enable_d3d9=false angle_enable_d3d11=false angle_enable_wgpu=false angle_enable_swiftshader=false angle_build_all=false use_custom_libcxx=false use_sysroot=true treat_warnings_as_errors=false enable_rust=false angle_has_histograms=false build_angle_deqp_tests=false'

ninja -C out/Release_arm64 libEGL libGLESv2

echo "=== Build complete ==="
ls -la out/Release_arm64/libEGL* out/Release_arm64/libGLESv2*
file out/Release_arm64/libEGL.so out/Release_arm64/libGLESv2.so
'''

with open('/home/catarina/angle_build/build_angle.sh', 'w', newline='\n') as f:
    f.write(script_content)

print("Script written successfully")
