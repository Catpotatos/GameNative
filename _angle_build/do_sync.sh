#!/bin/bash
export PATH="/home/catarina/angle_build/angle/third_party/depot_tools:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export DEPOT_TOOLS_UPDATE=0
export HOME=/home/catarina

cd /home/catarina/angle_build

LOG=/home/catarina/angle_build/android_sync.log

echo "=== gclient sync for Android ===" > $LOG
echo "Time: $(date)" >> $LOG
echo "gclient: $(which gclient)" >> $LOG
echo "" >> $LOG

gclient sync --nohooks -D >> $LOG 2>&1
RC=$?
echo "" >> $LOG
echo "=== Exit code: $RC ===" >> $LOG
echo "=== Done: $(date) ===" >> $LOG

