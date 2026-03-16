#!/bin/bash
# ============================================================================
# VirGL Three-Way Merge Helper
#
# Performs 3-way merges for all virgl source files.
# Run after setup_virgl_update.sh and after copying current files to current/
#
# Usage: bash merge_virgl_files.sh
# ============================================================================

set -euo pipefail

WORK_DIR="$HOME/virgl-update"
BASE_DIR="$WORK_DIR/base/virgl"
TARGET_DIR="$WORK_DIR/target/virgl"
CURRENT_DIR="$WORK_DIR/current"
MERGED_DIR="$WORK_DIR/merged"

# Files to merge (ordered by complexity, simplest first)
MERGE_FILES=(
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
    "vrend_decode.c"
    "vrend_renderer.h"
    "vrend_shader.c"
    "vrend_renderer.c"
)

# Files that are custom and should NOT be merged (just keep current)
CUSTOM_FILES=(
    "vrend_util.h"
)

echo "============================================"
echo "VirGL Three-Way Merge"
echo "============================================"
echo ""

# Verify directories exist
for dir in "$BASE_DIR" "$TARGET_DIR" "$CURRENT_DIR"; do
    if [ ! -d "$dir" ]; then
        echo "ERROR: Directory not found: $dir"
        echo "Run setup_virgl_update.sh first, then copy current files to $CURRENT_DIR"
        exit 1
    fi
done

mkdir -p "$MERGED_DIR"

# Track results
CLEAN_MERGES=()
CONFLICT_MERGES=()
MISSING_FILES=()

echo "Processing files..."
echo ""

for file in "${MERGE_FILES[@]}"; do
    base_file="$BASE_DIR/$file"
    target_file="$TARGET_DIR/$file"
    current_file="$CURRENT_DIR/$file"
    merged_file="$MERGED_DIR/$file"

    # Check all three versions exist
    if [ ! -f "$base_file" ]; then
        echo "⚠️  SKIP $file - not in base (new upstream file)"
        MISSING_FILES+=("$file (no base)")
        # If it exists in target but not base, it's a new file — just copy target
        if [ -f "$target_file" ]; then
            cp "$target_file" "$merged_file"
            echo "   → Copied target version to merged (new file, needs virgl_client mods)"
        fi
        continue
    fi

    if [ ! -f "$target_file" ]; then
        echo "⚠️  SKIP $file - not in target (removed upstream)"
        MISSING_FILES+=("$file (removed)")
        continue
    fi

    if [ ! -f "$current_file" ]; then
        echo "⚠️  SKIP $file - not in current (you may need to add it)"
        MISSING_FILES+=("$file (no current)")
        cp "$target_file" "$merged_file"
        echo "   → Copied target version to merged"
        continue
    fi

    # Perform three-way merge
    cp "$current_file" "$merged_file"

    if git merge-file -p "$merged_file" "$base_file" "$target_file" > "${merged_file}.tmp" 2>/dev/null; then
        mv "${merged_file}.tmp" "$merged_file"
        echo "✅ $file - merged cleanly"
        CLEAN_MERGES+=("$file")
    else
        mv "${merged_file}.tmp" "$merged_file"
        CONFLICTS=$(grep -c "^<<<<<<<" "$merged_file" 2>/dev/null || echo "0")
        echo "❌ $file - $CONFLICTS conflict(s) - needs manual resolution"
        CONFLICT_MERGES+=("$file ($CONFLICTS conflicts)")
    fi
done

# Copy custom files as-is
echo ""
echo "Copying custom files (no merge needed)..."
for file in "${CUSTOM_FILES[@]}"; do
    if [ -f "$CURRENT_DIR/$file" ]; then
        cp "$CURRENT_DIR/$file" "$MERGED_DIR/$file"
        echo "📋 $file - kept current version"
    fi
done

# Summary
echo ""
echo "============================================"
echo "MERGE SUMMARY"
echo "============================================"
echo ""
echo "Clean merges (${#CLEAN_MERGES[@]}):"
for f in "${CLEAN_MERGES[@]}"; do
    echo "  ✅ $f"
done

echo ""
echo "Conflicts (${#CONFLICT_MERGES[@]}):"
for f in "${CONFLICT_MERGES[@]}"; do
    echo "  ❌ $f"
done

echo ""
echo "Skipped/New (${#MISSING_FILES[@]}):"
for f in "${MISSING_FILES[@]}"; do
    echo "  ⚠️  $f"
done

echo ""
echo "Merged files are in: $MERGED_DIR/"
echo ""
if [ ${#CONFLICT_MERGES[@]} -gt 0 ]; then
    echo "⚠️  You have conflicts to resolve!"
    echo "Open each conflicted file and search for '<<<<<<<' markers."
    echo "For each conflict, choose the correct resolution:"
    echo "  - Keep Bruno's modification if it's a virgl_client/GLES change"
    echo "  - Accept upstream if it's a bug fix or new feature"
    echo "  - Combine both if they modify different aspects"
fi

