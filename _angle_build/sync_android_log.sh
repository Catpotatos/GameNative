#!/bin/bash

export PATH="/home/catarina/angle_build/angle/third_party/depot_tools:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export DEPOT_TOOLS_UPDATE=0
export HOME=/home/catarina

cd /home/catarina/angle_build

LOGFILE=/home/catarina/angle_build/android_sync.log

echo "=== Starting gclient sync for Android deps ===" > "$LOGFILE" 2>&1
echo "Time: $(date)" >> "$LOGFILE" 2>&1
echo "PATH: $PATH" >> "$LOGFILE" 2>&1
which gclient >> "$LOGFILE" 2>&1

gclient sync --nohooks -D >> "$LOGFILE" 2>&1
echo "=== gclient sync exit: $? ===" >> "$LOGFILE" 2>&1
echo "=== Done at $(date) ===" >> "$LOGFILE" 2>&1

