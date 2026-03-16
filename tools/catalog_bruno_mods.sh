#!/bin/bash
# ============================================================================
# Catalog Bruno's Modifications
#
# Compares upstream Mesa 23.1.9 virgl files against the current modified
# versions in the project to document all of Bruno's changes.
#
# Run after setup_virgl_update.sh has extracted the base files.
#
# Usage: bash catalog_bruno_mods.sh
# ============================================================================

set -euo pipefail

WORK_DIR="$HOME/virgl-update"
BASE_DIR="$WORK_DIR/base/virgl"
CURRENT_DIR="$WORK_DIR/current"
REPORT_DIR="$WORK_DIR/diffs/bruno_report"

mkdir -p "$REPORT_DIR"

# Files to compare
COMPARE_FILES=(
    "iov.c"
    "vrend_iov.h"
    "vrend_object.h"
    "vrend_object.c"
    "vrend_strbuf.h"
    "virgl_hw.h"
    "virgl_protocol.h"
    "vrend_blitter.h"
    "vrend_blitter.c"
    "vrend_formats.c"
    "vrend_shader.h"
    "vrend_shader.c"
    "vrend_decode.c"
    "vrend_renderer.h"
    "vrend_renderer.c"
)

# Custom files (no upstream equivalent)
CUSTOM_FILES=(
    "vrend_util.h"
)

echo "============================================"
echo "Bruno's Modifications Catalog"
echo "============================================"
echo ""

# Verify directories
if [ ! -d "$BASE_DIR" ]; then
    echo "ERROR: Base directory not found: $BASE_DIR"
    echo "Run setup_virgl_update.sh first."
    exit 1
fi

if [ ! -d "$CURRENT_DIR" ]; then
    echo "ERROR: Current directory not found: $CURRENT_DIR"
    echo "Copy your project's virgl src files to: $CURRENT_DIR"
    exit 1
fi

SUMMARY_FILE="$REPORT_DIR/SUMMARY.md"
echo "# Bruno's Modifications Summary" > "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"
echo "Generated: $(date)" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"
echo "| File | Lines Changed | Additions | Deletions | Key Changes |" >> "$SUMMARY_FILE"
echo "|---|---|---|---|---|" >> "$SUMMARY_FILE"

for file in "${COMPARE_FILES[@]}"; do
    base_file="$BASE_DIR/$file"
    current_file="$CURRENT_DIR/$file"

    if [ ! -f "$base_file" ]; then
        echo "⚠️  SKIP $file - not in base (may be new)"
        continue
    fi

    if [ ! -f "$current_file" ]; then
        echo "⚠️  SKIP $file - not in current project"
        continue
    fi

    # Generate unified diff
    diff -u "$base_file" "$current_file" > "$REPORT_DIR/$file.diff" 2>/dev/null || true

    # Count changes
    ADDITIONS=$(grep -c "^+" "$REPORT_DIR/$file.diff" 2>/dev/null || echo "0")
    DELETIONS=$(grep -c "^-" "$REPORT_DIR/$file.diff" 2>/dev/null || echo "0")
    TOTAL_CHANGED=$((ADDITIONS + DELETIONS))

    # Categorize changes
    VIRGL_CLIENT=$(grep -c "virgl_client" "$REPORT_DIR/$file.diff" 2>/dev/null || echo "0")
    GLES_REFS=$(grep -c -i "gles\|GL_ES\|gl2\.h\|gl3\.h" "$REPORT_DIR/$file.diff" 2>/dev/null || echo "0")
    ANDROID_LOG=$(grep -c "android_log\|__android" "$REPORT_DIR/$file.diff" 2>/dev/null || echo "0")

    KEY_CHANGES=""
    [ "$VIRGL_CLIENT" -gt 0 ] && KEY_CHANGES="${KEY_CHANGES}virgl_client($VIRGL_CLIENT) "
    [ "$GLES_REFS" -gt 0 ] && KEY_CHANGES="${KEY_CHANGES}GLES($GLES_REFS) "
    [ "$ANDROID_LOG" -gt 0 ] && KEY_CHANGES="${KEY_CHANGES}AndroidLog($ANDROID_LOG) "
    [ -z "$KEY_CHANGES" ] && KEY_CHANGES="minor/other"

    echo "📝 $file: +$ADDITIONS -$DELETIONS changes, key: $KEY_CHANGES"
    echo "| $file | $TOTAL_CHANGED | +$ADDITIONS | -$DELETIONS | $KEY_CHANGES |" >> "$SUMMARY_FILE"
done

echo ""
echo "Custom files (no upstream equivalent):"
for file in "${CUSTOM_FILES[@]}"; do
    if [ -f "$CURRENT_DIR/$file" ]; then
        LINES=$(wc -l < "$CURRENT_DIR/$file")
        echo "📋 $file: $LINES lines (entirely custom)"
        echo "| $file | N/A | N/A | N/A | Entirely custom |" >> "$SUMMARY_FILE"
        cp "$CURRENT_DIR/$file" "$REPORT_DIR/$file.full"
    fi
done

echo ""
echo "============================================"
echo "Full diff files saved to: $REPORT_DIR/"
echo "Summary: $SUMMARY_FILE"
echo "============================================"
echo ""

# Also generate a combined patch
echo "Generating combined patch..."
diff -ruN "$BASE_DIR/" "$CURRENT_DIR/" > "$WORK_DIR/diffs/bruno_all_mods.patch" 2>/dev/null || true
TOTAL_LINES=$(wc -l < "$WORK_DIR/diffs/bruno_all_mods.patch")
echo "Combined patch: $WORK_DIR/diffs/bruno_all_mods.patch ($TOTAL_LINES lines)"

