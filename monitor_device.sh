#!/bin/bash
# Real-time ADB monitoring dashboard for ANGLE Bionic testing

echo "=== ANGLE BIONIC TEST MONITORING DASHBOARD ==="
echo "Started: $(date)"
echo ""
echo "Monitoring streams:"
echo "  1. Full logcat → device_logs_full.txt"
echo "  2. System metrics → device_metrics.log"
echo "  3. Extraction logs below (live):"
echo ""

# Live extraction log monitoring
adb logcat TarCompressorUtils:I "ANGLE:*" "BionicProgram:*" -v threadtime

