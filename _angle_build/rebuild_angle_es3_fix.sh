#!/bin/bash
# =============================================================================
# Rebuild ANGLE with ES3 fix for Mali/non-provoking-vertex GPUs
# =============================================================================
#
# Problem: ANGLE's getMaxSupportedESVersion() limits to ES 2.0 when the Vulkan
# driver doesn't support VK_EXT_provoking_vertex. This blocks ES3 context
# creation on Mali GPUs with older drivers, causing VirGL to lose ALL ES3
# features (UBOs, VAOs, transform feedback, samplers, etc.)
#
# Fix: When angle_expose_non_conformant_extensions_and_versions is true (which
# it is in our build), skip the provoking vertex requirement. The provoking
# vertex issue only affects flat shading convention (last vs first vertex),
# which is irrelevant for D3D games via WineD3D.
#
# Run in WSL:  bash /mnt/c/Users/Catarina/StudioProjects/GameNative-mine/_angle_build/rebuild_angle_es3_fix.sh
# =============================================================================
set -e

export PATH="/usr/bin:/usr/local/bin:/usr/sbin:/bin:/sbin:/home/catarina/angle_build/angle/third_party/depot_tools:$PATH"

ANGLE_SRC="/home/catarina/angle_build/angle"
OUT_DIR="out/Android_arm64_7748"
PKG_DIR="/home/catarina/angle_build/package_7748_es3fix"
WIN_ASSETS="/mnt/c/Users/Catarina/StudioProjects/GameNative-mine/app/src/main/assets/graphics_driver"
LOG="/home/catarina/angle_build/build_7748_es3fix.log"
PATCH_FILE="/mnt/c/Users/Catarina/StudioProjects/GameNative-mine/_angle_build/patches/force_es3_provoking_vertex.patch"

echo "=== ANGLE ES3 Fix Build Script ===" | tee "$LOG"
echo "Started: $(date)" | tee -a "$LOG"

cd "$ANGLE_SRC"
echo "Branch: $(git branch --show-current)" | tee -a "$LOG"
echo "Commit: $(git log --oneline -1)" | tee -a "$LOG"

# =============================================================================
# Step 1: Apply the ES3 provoking vertex fix
# =============================================================================
echo "" | tee -a "$LOG"
echo "=== Step 1: Applying ES3 provoking vertex fix ===" | tee -a "$LOG"

# Check if the patch is already applied
VKRENDERER_FILE="src/libANGLE/renderer/vulkan/vk_renderer.cpp"
if grep -q 'exposeNonConformantExtensionsAndVersions.enabled' "$VKRENDERER_FILE" 2>/dev/null; then
    # Check specifically in the provokingVertex section
    PROVOKING_LINE=$(grep -n 'provokingVertex.enabled' "$VKRENDERER_FILE" | head -1 | cut -d: -f1)
    if [ -n "$PROVOKING_LINE" ]; then
        NEXT_LINES=$(sed -n "$((PROVOKING_LINE+1)),$((PROVOKING_LINE+5))p" "$VKRENDERER_FILE")
        if echo "$NEXT_LINES" | grep -q 'exposeNonConformantExtensionsAndVersions'; then
            echo "Patch already applied, skipping." | tee -a "$LOG"
        else
            echo "Applying patch manually..." | tee -a "$LOG"
            # Apply the fix manually using sed
            # Find the line: maxVersion = LimitVersionTo(maxVersion, {2, 0});
            # that follows: if (!mFeatures.provokingVertex.enabled)
            # and wrap it with the exposeNonConformant check
            sed -i '/VK_EXT_provoking_vertex is required for flat shading/,/LimitVersionTo(maxVersion, {2, 0})/ {
                s|maxVersion = LimitVersionTo(maxVersion, {2, 0});|if (!mFeatures.exposeNonConformantExtensionsAndVersions.enabled)\n            maxVersion = LimitVersionTo(maxVersion, {2, 0});|
            }' "$VKRENDERER_FILE"
            echo "Patch applied via sed." | tee -a "$LOG"
        fi
    fi
else
    echo "Applying patch manually..." | tee -a "$LOG"
    sed -i '/VK_EXT_provoking_vertex is required for flat shading/,/LimitVersionTo(maxVersion, {2, 0})/ {
        s|maxVersion = LimitVersionTo(maxVersion, {2, 0});|if (!mFeatures.exposeNonConformantExtensionsAndVersions.enabled)\n            maxVersion = LimitVersionTo(maxVersion, {2, 0});|
    }' "$VKRENDERER_FILE"
    echo "Patch applied via sed." | tee -a "$LOG"
fi

# Verify the patch
echo "Verifying patch..." | tee -a "$LOG"
PROVOKING_LINE=$(grep -n 'provokingVertex.enabled' "$VKRENDERER_FILE" | head -1 | cut -d: -f1)
echo "provokingVertex check at line $PROVOKING_LINE:" | tee -a "$LOG"
sed -n "$((PROVOKING_LINE-2)),$((PROVOKING_LINE+8))p" "$VKRENDERER_FILE" | tee -a "$LOG"

# =============================================================================
# Step 2: Build ANGLE
# =============================================================================
echo "" | tee -a "$LOG"
echo "=== Step 2: Building ANGLE (incremental) ===" | tee -a "$LOG"

# args.gn should already exist from previous build
if [ ! -f "$OUT_DIR/args.gn" ]; then
    echo "ERROR: $OUT_DIR/args.gn not found. Run build_angle_7748.sh first." | tee -a "$LOG"
    exit 1
fi

# Re-run GN gen to pick up any source changes
gn gen "$OUT_DIR" 2>&1 | tee -a "$LOG"

# Build
ninja -C "$OUT_DIR" libEGL libGLESv2 2>&1 | tee -a "$LOG"
echo "=== Build exit: $? ===" | tee -a "$LOG"

# =============================================================================
# Step 3: Verify
# =============================================================================
echo "" | tee -a "$LOG"
echo "=== Step 3: Verifying build ===" | tee -a "$LOG"
ls -la "$OUT_DIR"/libEGL_angle.so "$OUT_DIR"/libGLESv2_angle.so 2>&1 | tee -a "$LOG"
file "$OUT_DIR"/libEGL_angle.so 2>&1 | tee -a "$LOG"

# =============================================================================
# Step 4: Package
# =============================================================================
echo "" | tee -a "$LOG"
echo "=== Step 4: Packaging ===" | tee -a "$LOG"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/opt/angle/lib"

cp "$OUT_DIR/libEGL_angle.so" "$PKG_DIR/opt/angle/lib/libEGL.so"
cp "$OUT_DIR/libGLESv2_angle.so" "$PKG_DIR/opt/angle/lib/libGLESv2.so"

# Patch SONAMEs
patchelf --set-soname libEGL.so "$PKG_DIR/opt/angle/lib/libEGL.so"
patchelf --set-soname libGLESv2.so "$PKG_DIR/opt/angle/lib/libGLESv2.so"

echo "Packaged libraries:" | tee -a "$LOG"
ls -lh "$PKG_DIR/opt/angle/lib/" | tee -a "$LOG"
readelf -d "$PKG_DIR/opt/angle/lib/libEGL.so" | grep SONAME | tee -a "$LOG"
readelf -d "$PKG_DIR/opt/angle/lib/libGLESv2.so" | grep SONAME | tee -a "$LOG"

# =============================================================================
# Step 5: Create archive and deploy to assets
# =============================================================================
echo "" | tee -a "$LOG"
echo "=== Step 5: Creating archive ===" | tee -a "$LOG"
cd "$PKG_DIR"
tar cf - opt/angle | zstd -19 -o /home/catarina/angle_build/angle-7748-es3fix.tzst --force
echo "Archive created:" | tee -a "$LOG"
ls -lh /home/catarina/angle_build/angle-7748-es3fix.tzst | tee -a "$LOG"

echo "" | tee -a "$LOG"
echo "=== Step 6: Deploying to Android assets ===" | tee -a "$LOG"
mkdir -p "$WIN_ASSETS"
cp /home/catarina/angle_build/angle-7748-es3fix.tzst "$WIN_ASSETS/angle-7748.tzst"
echo "Deployed to: $WIN_ASSETS/angle-7748.tzst" | tee -a "$LOG"
ls -lh "$WIN_ASSETS/angle-7748.tzst" | tee -a "$LOG"

echo "" | tee -a "$LOG"
echo "=========================================="
echo "=== ANGLE ES3 FIX BUILD COMPLETE      ==="
echo "=========================================="
echo ""
echo "What changed:"
echo "  - Skipped VK_EXT_provoking_vertex requirement when"
echo "    exposeNonConformantExtensionsAndVersions is enabled"
echo "  - This allows ES3 context creation on GPUs without"
echo "    VK_EXT_provoking_vertex (Mali with older drivers)"
echo ""
echo "Next: Rebuild the APK in Android Studio and test."
echo "Expected log output:"
echo "  VirGL-ANGLE: ES3 context created successfully"
echo "  VirGL-ANGLE: Features: VAB=1 sync=1 UBO=1 TF=1 ..."

