#!/bin/bash
set +e  # Don't exit on errors, some diffs/files may not exist

WORK_DIR="$HOME/virgl-update"
TARGET_VREND="$WORK_DIR/target/virglrenderer/src/vrend"
TARGET_SRC="$WORK_DIR/target/virglrenderer/src"
CURRENT="$WORK_DIR/current"
DIFFS="$WORK_DIR/diffs"
OUT="/mnt/c/Users/Catarina/StudioProjects/GameNative-mine/correct_mapping.txt"

echo "=== Files in virglrenderer 1.3.0 src/ (top level) ===" > "$OUT"
ls "$TARGET_SRC"/virgl_hw.h "$TARGET_SRC"/virgl_protocol.h >> "$OUT" 2>&1 || echo "NOT FOUND" >> "$OUT"

echo "" >> "$OUT"
echo "=== Files in virglrenderer 1.3.0 src/vrend/ ===" >> "$OUT"
for f in "$TARGET_VREND"/*.c "$TARGET_VREND"/*.h; do
    basename "$f" >> "$OUT"
done

echo "" >> "$OUT"
echo "=== Current project files ===" >> "$OUT"
for f in "$CURRENT"/*.c "$CURRENT"/*.h; do
    basename "$f" >> "$OUT"
done

echo "" >> "$OUT"
echo "=== Generating corrected diffs ===" >> "$OUT"

# vrend files (in src/vrend/ in 1.3.0)
for file in vrend_renderer.c vrend_renderer.h vrend_shader.c vrend_shader.h \
            vrend_decode.c vrend_formats.c vrend_blitter.c vrend_blitter.h \
            vrend_object.c vrend_object.h iov.c vrend_iov.h vrend_strbuf.h; do
    if [ -f "$CURRENT/$file" ] && [ -f "$TARGET_VREND/$file" ]; then
        diff -u "$CURRENT/$file" "$TARGET_VREND/$file" > "$DIFFS/current_vs_target_$file.diff" 2>/dev/null || true
        LINES=$(wc -l < "$DIFFS/current_vs_target_$file.diff")
        echo "  $file: $LINES lines of diff" >> "$OUT"
    elif [ -f "$TARGET_VREND/$file" ]; then
        echo "  $file: NOT in current (new in 1.3.0)" >> "$OUT"
    elif [ -f "$CURRENT/$file" ]; then
        echo "  $file: NOT in virglrenderer 1.3.0 src/vrend/ (check other dirs)" >> "$OUT"
    else
        echo "  $file: MISSING from both" >> "$OUT"
    fi
done

# virgl_hw.h and virgl_protocol.h are in src/ not src/vrend/
echo "" >> "$OUT"
echo "=== Header files in src/ ===" >> "$OUT"
for file in virgl_hw.h virgl_protocol.h; do
    if [ -f "$CURRENT/$file" ] && [ -f "$TARGET_SRC/$file" ]; then
        diff -u "$CURRENT/$file" "$TARGET_SRC/$file" > "$DIFFS/current_vs_target_$file.diff" 2>/dev/null || true
        LINES=$(wc -l < "$DIFFS/current_vs_target_$file.diff")
        echo "  $file: $LINES lines of diff" >> "$OUT"
    fi
done

# Check TGSI support
echo "" >> "$OUT"
echo "=== TGSI references in virglrenderer 1.3.0 ===" >> "$OUT"
grep -rc "tgsi" "$TARGET_VREND/" 2>/dev/null | grep -v ":0$" | sed "s|$TARGET_VREND/||" >> "$OUT" || echo "  None found" >> "$OUT"

# Check the include/ directory structure
echo "" >> "$OUT"
echo "=== virglrenderer 1.3.0 include/ directory ===" >> "$OUT"
find "$WORK_DIR/target/virglrenderer" -path "*/include/*" -name "*.h" 2>/dev/null | sed "s|$WORK_DIR/target/virglrenderer/||" >> "$OUT" || echo "  Not found" >> "$OUT"

# Check the gallium dependency in Mesa 24.3.4
echo "" >> "$OUT"
echo "=== TGSI files in Mesa 24.3.4 gallium ===" >> "$OUT"
find "$WORK_DIR/target/mesa/src/gallium/auxiliary/tgsi" -name "*.c" 2>/dev/null | sed "s|$WORK_DIR/target/mesa/||" >> "$OUT" || echo "  Not found" >> "$OUT"

echo "" >> "$OUT"
echo "DONE" >> "$OUT"

