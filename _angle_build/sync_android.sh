#!/bin/bash
set -e

export PATH="/home/catarina/angle_build/angle/third_party/depot_tools:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export DEPOT_TOOLS_UPDATE=0

cd /home/catarina/angle_build

echo "=== Starting gclient sync for Android deps ==="
echo "PATH: $PATH"
echo "PWD: $PWD"
which gclient

gclient sync --nohooks -D 2>&1
echo "=== gclient sync exit: $? ==="

