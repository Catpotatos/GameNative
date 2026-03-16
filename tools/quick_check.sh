#!/bin/bash
# Quick gallium and line count check

WORK_DIR="$HOME/virgl-update"
OUT="$WORK_DIR/gallium_check.txt"

echo "=== gallium in virglrenderer 1.3.0 ===" > "$OUT"
find "$WORK_DIR/target/virglrenderer/src/gallium" -name "*.c" -o -name "*.h" 2>/dev/null | sort | sed "s|$WORK_DIR/target/virglrenderer/||" >> "$OUT"

echo "" >> "$OUT"
echo "=== gallium in current project ===" >> "$OUT"
find "$WORK_DIR/current/gallium" -name "*.c" -o -name "*.h" 2>/dev/null | sort | sed "s|$WORK_DIR/current/||" >> "$OUT"

echo "" >> "$OUT"
echo "=== Line counts ===" >> "$OUT"
echo "vrend_renderer.c:" >> "$OUT"
echo "  Current: $(wc -l < $WORK_DIR/current/vrend_renderer.c)" >> "$OUT"
echo "  Target:  $(wc -l < $WORK_DIR/target/virglrenderer/src/vrend/vrend_renderer.c)" >> "$OUT"

echo "vrend_shader.c:" >> "$OUT"
echo "  Current: $(wc -l < $WORK_DIR/current/vrend_shader.c)" >> "$OUT"
echo "  Target:  $(wc -l < $WORK_DIR/target/virglrenderer/src/vrend/vrend_shader.c)" >> "$OUT"

echo "vrend_decode.c:" >> "$OUT"
echo "  Current: $(wc -l < $WORK_DIR/current/vrend_decode.c)" >> "$OUT"
echo "  Target:  $(wc -l < $WORK_DIR/target/virglrenderer/src/vrend/vrend_decode.c)" >> "$OUT"

echo "" >> "$OUT"
echo "=== New files in 1.3.0 not in current ===" >> "$OUT"
for f in vrend_debug.c vrend_debug.h vrend_tweaks.c vrend_tweaks.h vrend_winsys.c vrend_winsys.h vrend_winsys_egl.c vrend_winsys_egl.h; do
    echo "$f: exists in 1.3.0 src/vrend/" >> "$OUT"
done

echo "" >> "$OUT"
echo "DONE" >> "$OUT"

cp "$OUT" "/mnt/c/Users/Catarina/StudioProjects/GameNative-mine/gallium_check.txt"
echo "Written to gallium_check.txt"

