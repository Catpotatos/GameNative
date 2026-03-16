#!/bin/bash
# ============================================================================
# VirGL Update Helper Script
#
# This script automates the setup for updating VirGL from Mesa 23.1.9 to 24.3.x
# Run this in WSL2 or a Linux environment.
#
# Usage: bash setup_virgl_update.sh [mesa_target_tag]
# Example: bash setup_virgl_update.sh mesa-24.3.4
# ============================================================================

set -euo pipefail

# Configuration
MESA_BASE_TAG="mesa-23.1.9"
MESA_TARGET_TAG="${1:-mesa-24.3.4}"  # Default to 24.3.4, override via argument
WORK_DIR="$HOME/virgl-update"
MESA_REPO="https://gitlab.freedesktop.org/mesa/mesa.git"

# Paths within Mesa that we need
VIRGL_PATH="src/virgl"
GALLIUM_AUX_PATH="src/gallium/auxiliary"
GALLIUM_INC_PATH="src/gallium/include"

echo "============================================"
echo "VirGL Update Setup Script"
echo "============================================"
echo "Base version:   $MESA_BASE_TAG"
echo "Target version: $MESA_TARGET_TAG"
echo "Work directory: $WORK_DIR"
echo "============================================"
echo ""

# Step 1: Create work directory
echo "[1/7] Creating work directory..."
mkdir -p "$WORK_DIR"/{base,target,current,diffs,merged}
cd "$WORK_DIR"

# Step 2: Clone Mesa (shallow, just enough for the two tags)
if [ ! -d "mesa" ]; then
    echo "[2/7] Cloning Mesa repository (this may take a while)..."
    git clone --no-checkout --filter=blob:none "$MESA_REPO"
else
    echo "[2/7] Mesa repo already exists, updating..."
    cd mesa && git fetch && cd ..
fi

cd mesa

# Step 3: Extract base version files
echo "[3/7] Extracting base version ($MESA_BASE_TAG) files..."
git checkout "$MESA_BASE_TAG" -- "$VIRGL_PATH" "$GALLIUM_AUX_PATH" "$GALLIUM_INC_PATH" 2>/dev/null || {
    echo "ERROR: Could not checkout $MESA_BASE_TAG. Available virgl-related tags:"
    git tag | grep -E "mesa-23\.|mesa-24\." | head -20
    exit 1
}
cp -r "$VIRGL_PATH" "$WORK_DIR/base/virgl"
cp -r "$GALLIUM_AUX_PATH" "$WORK_DIR/base/gallium_auxiliary"
cp -r "$GALLIUM_INC_PATH" "$WORK_DIR/base/gallium_include"
git checkout -- .
git clean -fd "$VIRGL_PATH" "$GALLIUM_AUX_PATH" "$GALLIUM_INC_PATH" 2>/dev/null || true

# Step 4: Extract target version files
echo "[4/7] Extracting target version ($MESA_TARGET_TAG) files..."
git checkout "$MESA_TARGET_TAG" -- "$VIRGL_PATH" "$GALLIUM_AUX_PATH" "$GALLIUM_INC_PATH" 2>/dev/null || {
    echo "ERROR: Could not checkout $MESA_TARGET_TAG. Available tags:"
    git tag | grep -E "mesa-24\.|mesa-25\.|mesa-26\." | head -30
    exit 1
}
cp -r "$VIRGL_PATH" "$WORK_DIR/target/virgl"
cp -r "$GALLIUM_AUX_PATH" "$WORK_DIR/target/gallium_auxiliary"
cp -r "$GALLIUM_INC_PATH" "$WORK_DIR/target/gallium_include"
git checkout -- .
git clean -fd "$VIRGL_PATH" "$GALLIUM_AUX_PATH" "$GALLIUM_INC_PATH" 2>/dev/null || true

cd "$WORK_DIR"

# Step 5: Generate upstream diff
echo "[5/7] Generating upstream change diff..."
cd mesa
git diff "$MESA_BASE_TAG..$MESA_TARGET_TAG" -- "$VIRGL_PATH/" > "$WORK_DIR/diffs/upstream_virgl_changes.patch" 2>/dev/null || true
git diff "$MESA_BASE_TAG..$MESA_TARGET_TAG" -- "$GALLIUM_AUX_PATH/" "$GALLIUM_INC_PATH/" > "$WORK_DIR/diffs/upstream_gallium_changes.patch" 2>/dev/null || true

# File stat summary
git diff "$MESA_BASE_TAG..$MESA_TARGET_TAG" --stat -- "$VIRGL_PATH/" > "$WORK_DIR/diffs/upstream_virgl_stat.txt" 2>/dev/null || true

# Commit log for virgl changes
git log --oneline "$MESA_BASE_TAG..$MESA_TARGET_TAG" -- "$VIRGL_PATH/" > "$WORK_DIR/diffs/upstream_virgl_commits.txt" 2>/dev/null || true

cd "$WORK_DIR"

# Step 6: Check TGSI support in target
echo "[6/7] Checking TGSI support in target version..."
if grep -r "tgsi" "$WORK_DIR/target/virgl/" > /dev/null 2>&1; then
    TGSI_COUNT=$(grep -r "tgsi" "$WORK_DIR/target/virgl/" | wc -l)
    echo "  ✅ TGSI references found in target: $TGSI_COUNT occurrences"
else
    echo "  ⚠️  WARNING: No TGSI references found in target! TGSI may have been removed."
    echo "  Consider using an older target version."
fi

# Check for new .c files in virgl
echo ""
echo "  Files in base virgl:"
find "$WORK_DIR/base/virgl" -name "*.c" | sort | sed 's|.*/||' | tee "$WORK_DIR/diffs/base_c_files.txt"
echo ""
echo "  Files in target virgl:"
find "$WORK_DIR/target/virgl" -name "*.c" | sort | sed 's|.*/||' | tee "$WORK_DIR/diffs/target_c_files.txt"
echo ""

# Show new/removed files
echo "  New .c files in target (not in base):"
comm -13 "$WORK_DIR/diffs/base_c_files.txt" "$WORK_DIR/diffs/target_c_files.txt" || echo "  (none)"
echo ""
echo "  Removed .c files from base (not in target):"
comm -23 "$WORK_DIR/diffs/base_c_files.txt" "$WORK_DIR/diffs/target_c_files.txt" || echo "  (none)"

# Step 7: Instructions
echo ""
echo "[7/7] Setup complete!"
echo ""
echo "============================================"
echo "NEXT STEPS"
echo "============================================"
echo ""
echo "1. Copy your current project's virgl files to: $WORK_DIR/current/"
echo "   cp -r /path/to/GameNative-mine/app/src/main/cpp/virglrenderer/src/* $WORK_DIR/current/"
echo ""
echo "2. Generate Bruno's modification diff:"
echo "   diff -ruN $WORK_DIR/base/virgl/ $WORK_DIR/current/ > $WORK_DIR/diffs/bruno_mods.patch"
echo ""
echo "3. For each file, perform 3-way merge:"
echo "   For example, for vrend_renderer.c:"
echo "   cp $WORK_DIR/current/vrend_renderer.c $WORK_DIR/merged/vrend_renderer.c"
echo "   git merge-file $WORK_DIR/merged/vrend_renderer.c \\"
echo "     $WORK_DIR/base/virgl/vrend_renderer.c \\"
echo "     $WORK_DIR/target/virgl/vrend_renderer.c"
echo ""
echo "4. Review diffs in: $WORK_DIR/diffs/"
echo "   - upstream_virgl_changes.patch  (what changed upstream)"
echo "   - upstream_virgl_stat.txt       (summary of changes)"
echo "   - upstream_virgl_commits.txt    (commit messages)"
echo "   - upstream_gallium_changes.patch (gallium changes)"
echo ""
echo "5. After merging, copy results back to your project:"
echo "   cp $WORK_DIR/merged/* /path/to/GameNative-mine/app/src/main/cpp/virglrenderer/src/"
echo ""
echo "============================================"

