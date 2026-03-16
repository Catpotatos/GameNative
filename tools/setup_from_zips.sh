#!/bin/bash
# ============================================================================
# VirGL Update - Setup from Zip Files
#
# Uses the downloaded zip files instead of cloning repos.
# Designed to work with the files the user already has.
#
# Usage: bash setup_from_zips.sh
# ============================================================================

set -euo pipefail

WORK_DIR="$HOME/virgl-update"
WIN_DOWNLOADS="/mnt/c/Users/Catarina/Downloads"
WIN_PROJECT="/mnt/c/Users/Catarina/StudioProjects/GameNative-mine"
VIRGL_SRC="$WIN_PROJECT/app/src/main/cpp/virglrenderer"

echo "============================================"
echo "VirGL Update - Setup from Zip Files"
echo "============================================"
echo ""

# Step 1: Create work directory structure
echo "[1/6] Creating work directory structure..."
mkdir -p "$WORK_DIR"/{base,target,current,diffs,merged}
mkdir -p "$WORK_DIR/base/virglrenderer"
mkdir -p "$WORK_DIR/target/virglrenderer"
mkdir -p "$WORK_DIR/target/mesa"

# Step 2: Extract virglrenderer 1.3.0 (the new upstream target for virgl core)
echo "[2/6] Extracting virglrenderer 1.3.0..."
if [ ! -f "$WORK_DIR/target/virglrenderer/.extracted" ]; then
    # Use the virglrenderer-virglrenderer-1.3.0.zip which has the clean structure
    unzip -q -o "$WIN_DOWNLOADS/virglrenderer-virglrenderer-1.3.0.zip" -d "$WORK_DIR/target/virglrenderer_tmp"
    # Find the actual root dir inside the zip
    VIRGL_ROOT=$(find "$WORK_DIR/target/virglrenderer_tmp" -maxdepth 1 -type d | tail -1)
    if [ -d "$VIRGL_ROOT/src" ]; then
        cp -r "$VIRGL_ROOT"/* "$WORK_DIR/target/virglrenderer/"
    else
        # Nested one level deeper
        VIRGL_ROOT=$(find "$WORK_DIR/target/virglrenderer_tmp" -maxdepth 2 -name "src" -type d | head -1)
        VIRGL_ROOT=$(dirname "$VIRGL_ROOT")
        cp -r "$VIRGL_ROOT"/* "$WORK_DIR/target/virglrenderer/"
    fi
    rm -rf "$WORK_DIR/target/virglrenderer_tmp"
    touch "$WORK_DIR/target/virglrenderer/.extracted"
    echo "  ✅ Extracted to $WORK_DIR/target/virglrenderer/"
else
    echo "  ✅ Already extracted"
fi

# Step 3: Extract Mesa 24.3.4 (for updated gallium utilities)
echo "[3/6] Extracting Mesa 24.3.4..."
if [ ! -f "$WORK_DIR/target/mesa/.extracted" ]; then
    unzip -q -o "$WIN_DOWNLOADS/mesa-mesa-24.3.4.zip" -d "$WORK_DIR/target/mesa_tmp"
    MESA_ROOT=$(find "$WORK_DIR/target/mesa_tmp" -maxdepth 1 -type d | tail -1)
    if [ -d "$MESA_ROOT/src/gallium" ]; then
        cp -r "$MESA_ROOT"/* "$WORK_DIR/target/mesa/"
    else
        MESA_ROOT=$(find "$WORK_DIR/target/mesa_tmp" -maxdepth 2 -name "src" -type d | head -1)
        MESA_ROOT=$(dirname "$MESA_ROOT")
        cp -r "$MESA_ROOT"/* "$WORK_DIR/target/mesa/"
    fi
    rm -rf "$WORK_DIR/target/mesa_tmp"
    touch "$WORK_DIR/target/mesa/.extracted"
    echo "  ✅ Extracted to $WORK_DIR/target/mesa/"
else
    echo "  ✅ Already extracted"
fi

# Step 4: Copy current project files
echo "[4/6] Copying current project virgl files..."
cp -r "$VIRGL_SRC/src/"* "$WORK_DIR/current/"
echo "  ✅ Copied to $WORK_DIR/current/"

# Step 5: Identify the base version
# Bruno's code is based on virglrenderer ~0.9.x/0.10.x era (Mesa 23.1.9 timeframe)
# We'll use the virglrenderer 1.3.0 source as the TARGET
# For the BASE, we need the version Bruno started from.
# Since we don't have the exact base, we can still diff current vs target.
echo "[5/6] Analyzing file structures..."

echo "" > "$WORK_DIR/diffs/structure_analysis.txt"
echo "=== Current project virgl .c/.h files ===" >> "$WORK_DIR/diffs/structure_analysis.txt"
find "$WORK_DIR/current" -name "*.c" -o -name "*.h" | sort | sed "s|$WORK_DIR/current/||" >> "$WORK_DIR/diffs/structure_analysis.txt"

echo "" >> "$WORK_DIR/diffs/structure_analysis.txt"
echo "=== virglrenderer 1.3.0 src/ .c/.h files ===" >> "$WORK_DIR/diffs/structure_analysis.txt"
find "$WORK_DIR/target/virglrenderer/src" -maxdepth 1 \( -name "*.c" -o -name "*.h" \) | sort | sed "s|$WORK_DIR/target/virglrenderer/src/||" >> "$WORK_DIR/diffs/structure_analysis.txt"

echo "" >> "$WORK_DIR/diffs/structure_analysis.txt"
echo "=== Mesa 24.3.4 gallium auxiliary .c files (relevant subset) ===" >> "$WORK_DIR/diffs/structure_analysis.txt"
for f in u_format.c u_format_table.c u_texture.c u_hash_table.c u_debug.c u_cpu_detect.c u_bitmask.c u_surface.c u_math.c u_debug_describe.c; do
    find "$WORK_DIR/target/mesa/src/gallium/auxiliary" -name "$f" 2>/dev/null | sed "s|$WORK_DIR/target/mesa/||" >> "$WORK_DIR/diffs/structure_analysis.txt"
done
for f in cso_cache.c cso_hash.c; do
    find "$WORK_DIR/target/mesa/src/gallium/auxiliary" -name "$f" 2>/dev/null | sed "s|$WORK_DIR/target/mesa/||" >> "$WORK_DIR/diffs/structure_analysis.txt"
done
for f in tgsi_dump.c tgsi_ureg.c tgsi_build.c tgsi_scan.c tgsi_info.c tgsi_parse.c tgsi_text.c tgsi_strings.c tgsi_sanity.c tgsi_iterate.c tgsi_util.c tgsi_transform.c; do
    find "$WORK_DIR/target/mesa/src/gallium/auxiliary" -name "$f" 2>/dev/null | sed "s|$WORK_DIR/target/mesa/||" >> "$WORK_DIR/diffs/structure_analysis.txt"
done

# Step 6: Check TGSI support
echo "[6/6] Checking TGSI support..."
echo "" >> "$WORK_DIR/diffs/structure_analysis.txt"
echo "=== TGSI Support Check ===" >> "$WORK_DIR/diffs/structure_analysis.txt"

TGSI_IN_VR=$(grep -rc "tgsi" "$WORK_DIR/target/virglrenderer/src/" 2>/dev/null | grep -v ":0$" || echo "none")
echo "TGSI references in virglrenderer 1.3.0:" >> "$WORK_DIR/diffs/structure_analysis.txt"
echo "$TGSI_IN_VR" >> "$WORK_DIR/diffs/structure_analysis.txt"

TGSI_IN_MESA=$(find "$WORK_DIR/target/mesa/src/gallium/auxiliary/tgsi" -name "*.c" 2>/dev/null | wc -l)
echo "" >> "$WORK_DIR/diffs/structure_analysis.txt"
echo "TGSI .c files in Mesa 24.3.4 gallium: $TGSI_IN_MESA" >> "$WORK_DIR/diffs/structure_analysis.txt"

# Generate diffs between current and target virglrenderer
echo ""
echo "Generating diffs between current project and virglrenderer 1.3.0..."
TARGET_VIRGL="$WORK_DIR/target/virglrenderer/src"

for file in vrend_renderer.c vrend_renderer.h vrend_shader.c vrend_shader.h \
            vrend_decode.c vrend_formats.c vrend_blitter.c vrend_blitter.h \
            vrend_object.c vrend_object.h iov.c vrend_iov.h vrend_strbuf.h \
            virgl_hw.h virgl_protocol.h; do
    if [ -f "$WORK_DIR/current/$file" ] && [ -f "$TARGET_VIRGL/$file" ]; then
        diff -u "$WORK_DIR/current/$file" "$TARGET_VIRGL/$file" > "$WORK_DIR/diffs/current_vs_target_$file.diff" 2>/dev/null || true
        LINES=$(wc -l < "$WORK_DIR/diffs/current_vs_target_$file.diff")
        echo "  $file: $LINES lines of diff"
    elif [ -f "$TARGET_VIRGL/$file" ]; then
        echo "  $file: NOT in current (new in 1.3.0)"
    elif [ -f "$WORK_DIR/current/$file" ]; then
        echo "  $file: NOT in virglrenderer 1.3.0 (custom/removed)"
    fi
done

# Copy analysis file to Windows for easy viewing
cp "$WORK_DIR/diffs/structure_analysis.txt" "$WIN_PROJECT/structure_analysis.txt"

echo ""
echo "============================================"
echo "SETUP COMPLETE"
echo "============================================"
echo ""
echo "Work directory: $WORK_DIR"
echo "  base/          - (needs git clone for base version)"
echo "  target/"
echo "    virglrenderer/ - virglrenderer 1.3.0 source"
echo "    mesa/          - Mesa 24.3.4 source (for gallium)"
echo "  current/       - Your current project files"
echo "  diffs/         - Generated diffs and analysis"
echo "  merged/        - (for merge results)"
echo ""
echo "Analysis file copied to project: structure_analysis.txt"
echo ""
echo "Next steps:"
echo "1. Review structure_analysis.txt"
echo "2. Review diffs in $WORK_DIR/diffs/"
echo "3. For each file, decide: update from virglrenderer 1.3.0 or keep current"
echo "============================================"

