#!/bin/bash
# Build GL4ES for Android/Bionic (arm64) with GLX support using ANGLE's clang toolchain.
#
# IMPORTANT: gl4es MUST be built with GLX support (not stubs) for the ANGLE rendering
# pipeline to work. Wine uses GLX (via WINE_X11FORCEGLX=1) to initialize OpenGL.
# gl4es's GLX implementation wraps EGL calls internally.
#
# The rendering chain is:
#   Game → DirectX → wined3d → OpenGL/GLX → gl4es (GLX→EGL) → ANGLE (GLES→Vulkan) → GPU
#
# Without real GLX support (i.e., with -DGLX_STUBS=ON), Wine gets 0 pixel formats
# from glXGetFBConfigs, wined3d fails to create an adapter, and Direct3D initialization
# fails with hr 0x80004005 (black screen).
#
# Prerequisites:
#   1. Build the X11 stub library first:
#      cd x11_stubs && bash build_x11_stubs.sh
#   2. This creates x11_stubs/lib/libX11.so (minimal bionic stub)
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CLANG="/home/catarina/angle_build/angle/third_party/llvm-build/Release+Asserts/bin/clang"
SYSROOT="/home/catarina/angle_build/angle/third_party/android_toolchain/ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
GL4ES_SRC="/home/catarina/gl4es_build/gl4es"
BUILD_DIR="/home/catarina/gl4es_build/build_bionic"

# Paths to our minimal X11 stubs (headers + stub library)
X11_STUBS_DIR="$SCRIPT_DIR/x11_stubs"
X11_INCLUDE="$X11_STUBS_DIR/include"
X11_LIB="$X11_STUBS_DIR/lib"

# Verify X11 stubs are built
if [ ! -f "$X11_LIB/libX11.so" ]; then
    echo "ERROR: X11 stub library not found at $X11_LIB/libX11.so"
    echo "Build it first: cd x11_stubs && bash build_x11_stubs.sh"
    exit 1
fi

# Also create a static archive (.a) of the X11 stub for static linking into GL4ES.
# This prevents deploying a separate libX11.so that would shadow Wine's real
# libX11.so in LD_LIBRARY_PATH. GL4ES gets the stubs baked in; Wine keeps the
# real X11 client library for communicating with the GameNative X server.
echo "=== Creating static X11 stub archive for linking into GL4ES ==="
X11_STATIC_DIR="$X11_STUBS_DIR/lib_static"
mkdir -p "$X11_STATIC_DIR"
AR="$CLANG/../llvm-ar"
if [ ! -f "$AR" ]; then
    AR="$(dirname "$CLANG")/llvm-ar"
fi
# Compile stub to object file
"$CLANG" --target=aarch64-linux-android28 --sysroot="$SYSROOT" \
    -O2 -fPIC -I"$X11_INCLUDE" -c "$X11_STUBS_DIR/x11_stub.c" \
    -o "$X11_STATIC_DIR/x11_stub.o"
# Create static archive
"$AR" rcs "$X11_STATIC_DIR/libX11.a" "$X11_STATIC_DIR/x11_stub.o"
echo "Static archive: $X11_STATIC_DIR/libX11.a"

echo "=== Building GL4ES for Android/Bionic (arm64) with GLX support ==="
echo "X11 stub headers: $X11_INCLUDE"
echo "X11 stub library: $X11_LIB/libX11.so"

# Create a cmake toolchain file to avoid CMake's built-in Android detection
TOOLCHAIN_FILE="$BUILD_DIR/toolchain.cmake"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

cat > "$TOOLCHAIN_FILE" << 'EOF'
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER_TARGET aarch64-linux-android28)
set(CMAKE_C_FLAGS_INIT "--target=aarch64-linux-android28 -fPIC")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "--target=aarch64-linux-android28 --rtlib=compiler-rt --unwindlib=none")
set(CMAKE_EXE_LINKER_FLAGS_INIT "--target=aarch64-linux-android28 --rtlib=compiler-rt --unwindlib=none")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
EOF

cd "$BUILD_DIR"

# NOTE on build flags:
#   - NOX11 is OFF (default) → enables gl4es's real GLX implementation that wraps EGL
#   - GLX_STUBS is OFF (default) → uses real GLX code, not empty stubs
#   - ANDROID is intentionally NOT set — gl4es's CMakeLists.txt treats ANDROID=ON
#     as implying NOX11=ON (disabling GLX entirely), which breaks the ANGLE pipeline.
#     The toolchain file already targets aarch64-linux-android28, so Android-specific
#     code paths are selected by the compiler target, not the CMake variable.
#   - USE_ANDROID_LOG=ON → enables __android_log_print (replaces -DANDROID=ON for logging)
#   - X11_X11_LIB points to our minimal stub library
#   - CMAKE_C_FLAGS includes our minimal X11 headers
#
# gl4es's CMake find_package(X11) needs:
#   X11_X11_INCLUDE_PATH - path to X11 headers
#   X11_X11_LIB - path to libX11.a (static archive — baked into libGL.so.1)
#
# IMPORTANT: We use the static archive (libX11.a) not the shared library (libX11.so).
# Deploying libX11.so alongside GL4ES in /opt/gl4es/lib/ would shadow Wine's real
# libX11.so (in /usr/lib/) since /opt/gl4es/lib comes first in LD_LIBRARY_PATH.
# That would break Wine's X11 communication with the GameNative X server.
cmake "$GL4ES_SRC" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_C_COMPILER="$CLANG" \
    -DCMAKE_SYSROOT="$SYSROOT" \
    -DCMAKE_BUILD_TYPE=Release \
    -DNOX11=OFF \
    -DGLX_STUBS=OFF \
    -DUSE_ANDROID_LOG=ON \
    -DX11_FOUND=TRUE \
    -DX11_X11_INCLUDE_PATH="$X11_INCLUDE" \
    -DX11_INCLUDE_DIR="$X11_INCLUDE" \
    -DX11_X11_LIB="$X11_STATIC_DIR/libX11.a" \
    -DX11_LIBRARIES="$X11_STATIC_DIR/libX11.a" \
    -Dlog-lib=$SYSROOT/usr/lib/aarch64-linux-android/28/liblog.so \
    -DCMAKE_C_FLAGS="-O2 -fPIC --target=aarch64-linux-android28 -I$X11_INCLUDE -Wno-int-conversion -Wno-return-mismatch -Wno-incompatible-pointer-types" \
    -DCMAKE_SHARED_LINKER_FLAGS="--target=aarch64-linux-android28 --rtlib=compiler-rt --unwindlib=none -L$X11_STATIC_DIR -Wl,--whole-archive -lX11 -Wl,--no-whole-archive -Wl,-soname,libGL.so.1"

echo "=== Compiling... ==="
make -j$(nproc)

echo "=== Build complete ==="
OUTPUT="$GL4ES_SRC/lib/libGL.so"
ls -la "$OUTPUT"

echo ""
echo "=== Verifying bionic linkage ==="
readelf -d "$OUTPUT" | grep -E "NEEDED|SONAME"

echo ""
echo "=== Checking architecture ==="
file "$OUTPUT"

echo ""
echo "=== Verifying GLX symbols (CRITICAL for ANGLE pipeline) ==="
echo "Must have glXChooseVisual, glXGetFBConfigs, glXCreateContext, etc."
GLX_COUNT=$(readelf -Ws "$OUTPUT" | grep -ci "glX" || true)
echo "Found $GLX_COUNT glX* symbols"
if [ "$GLX_COUNT" -lt 5 ]; then
    echo "⚠️  WARNING: Too few GLX symbols ($GLX_COUNT). GLX may not be compiled in!"
    echo "   This means Wine cannot initialize OpenGL through GL4ES."
    echo "   Check if -DANDROID=ON was forcing NOX11=ON in CMakeLists.txt."
    readelf -Ws "$OUTPUT" | grep -i "glX" || echo "   (no glX symbols found at all)"
else
    echo "✅ GLX support looks good. Key functions:"
    readelf -Ws "$OUTPUT" | grep -i "glXChooseVisual\|glXGetFBConfig\|glXCreateContext\|glXMakeCurrent\|glXSwapBuffers"
fi

# Copy outputs — only GL4ES (X11 stubs are statically linked into it).
# Do NOT deploy a separate libX11.so here — it would shadow Wine's real
# libX11.so since /opt/gl4es/lib comes first in LD_LIBRARY_PATH.
mkdir -p "$BUILD_DIR/output/opt/gl4es/lib"
cp "$OUTPUT" "$BUILD_DIR/output/opt/gl4es/lib/libGL.so.1"

# Verify X11 stubs are baked into libGL.so.1
echo ""
echo "=== Verifying X11 stubs are statically linked ==="
X11_SYM_COUNT=$(readelf -Ws "$OUTPUT" | grep -cE "XOpenDisplay|XGetVisualInfo|XCreateColormap" || true)
echo "Found $X11_SYM_COUNT X11 stub symbols in libGL.so.1"
if [ "$X11_SYM_COUNT" -lt 3 ]; then
    echo "⚠️  WARNING: X11 stubs may not be statically linked. Check build log."
    echo "   Falling back to deploying separate libX11.so (may shadow Wine's X11)"
    cp "$X11_LIB/libX11.so" "$BUILD_DIR/output/opt/gl4es/lib/libX11.so"
fi

echo ""
echo "=== Output files ==="
ls -la "$BUILD_DIR/output/opt/gl4es/lib/"
echo ""
echo "libGL.so.1 (gl4es with GLX + X11 stubs statically linked) is in:"
echo "  $BUILD_DIR/output/opt/gl4es/lib/"
echo ""
echo "NOTE: libX11.so is NOT deployed separately. The X11 stubs are statically"
echo "linked into libGL.so.1 to avoid shadowing Wine's real libX11.so in"
echo "LD_LIBRARY_PATH. The GameNative X server now has a GLX extension stub"
echo "that responds to GLX queries from Wine/GL4ES."
echo ""
echo "Package these into gl4es-bionic-*.tzst:"
echo "  cd $BUILD_DIR/output && tar cf - opt/gl4es | zstd -19 -o gl4es-bionic-1.1.8.tzst"
echo ""
echo "Then copy the .tzst to app/src/main/assets/graphics_driver/"
echo "and update DefaultVersion.GL4ES to '1.1.8' to force re-extraction."
echo "=== Done ==="
