#!/bin/bash
# ============================================================================
# Check if a target Mesa version still has TGSI support in virgl
# This is critical - if TGSI is removed, the update approach changes entirely.
#
# Usage: bash check_tgsi_support.sh mesa-24.3.4
# ============================================================================

set -euo pipefail

TAG="${1:-mesa-24.3.4}"
MESA_DIR="${2:-$HOME/virgl-update/mesa}"

if [ ! -d "$MESA_DIR" ]; then
    echo "ERROR: Mesa repo not found at $MESA_DIR"
    echo "Run setup_virgl_update.sh first"
    exit 1
fi

cd "$MESA_DIR"

echo "Checking TGSI support in $TAG..."
echo ""

# Check out the tag temporarily
git show "$TAG:src/virgl/vrend_shader.c" > /dev/null 2>&1 || {
    echo "ERROR: Could not find src/virgl/vrend_shader.c in $TAG"
    echo "The virgl directory may have been restructured."
    echo ""
    echo "Checking for virgl files..."
    git ls-tree -r --name-only "$TAG" | grep "virgl" | head -20
    exit 1
}

echo "✅ vrend_shader.c exists in $TAG"

# Count TGSI references
TGSI_COUNT=$(git show "$TAG:src/virgl/vrend_shader.c" | grep -c "tgsi" || true)
echo "   TGSI references in vrend_shader.c: $TGSI_COUNT"

TGSI_INCLUDE=$(git show "$TAG:src/virgl/vrend_shader.c" | grep "#include.*tgsi" || true)
echo "   TGSI includes: $TGSI_INCLUDE"

# Check vrend_renderer.c too
TGSI_RENDERER=$(git show "$TAG:src/virgl/vrend_renderer.c" | grep -c "tgsi" || true)
echo "   TGSI references in vrend_renderer.c: $TGSI_RENDERER"

# Check vrend_decode.c
TGSI_DECODE=$(git show "$TAG:src/virgl/vrend_decode.c" | grep -c "tgsi" || true)
echo "   TGSI references in vrend_decode.c: $TGSI_DECODE"

echo ""
if [ "$TGSI_COUNT" -gt 50 ]; then
    echo "✅ TGSI support appears INTACT in $TAG (good for update)"
elif [ "$TGSI_COUNT" -gt 10 ]; then
    echo "⚠️  TGSI support may be PARTIAL in $TAG (investigate further)"
else
    echo "❌ TGSI support appears REMOVED or MINIMAL in $TAG"
    echo "   Consider using an earlier version."
fi

# Also list all .c files in virgl
echo ""
echo "Source files in $TAG:src/virgl/:"
git ls-tree --name-only "$TAG" src/virgl/ | grep "\.c$" | sed 's|src/virgl/||'

echo ""
echo "Header files in $TAG:src/virgl/:"
git ls-tree --name-only "$TAG" src/virgl/ | grep "\.h$" | sed 's|src/virgl/||'

