#!/bin/bash
# Build GL4ES for Android/Bionic (arm64) with GLX support.
# Run this in WSL Ubuntu.
set -e

CLANG="/home/catarina/angle_build/angle/third_party/llvm-build/Release+Asserts/bin/clang"
SYSROOT="/home/catarina/angle_build/angle/third_party/android_toolchain/ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
GL4ES_SRC="/home/catarina/gl4es_build/gl4es"
BUILD_DIR="/home/catarina/gl4es_build/build_bionic"
X11_STUBS_DIR="/home/catarina/gl4es_build/x11_stubs"
X11_INCLUDE="$X11_STUBS_DIR/include"
X11_LIB="$X11_STUBS_DIR/lib"

# Where to copy the final .tzst
WIN_ASSETS="/mnt/c/Users/Catarina/StudioProjects/GameNative-mine/app/src/main/assets/graphics_driver"

echo "=== Step 1: Creating static X11 stub archive ==="
X11_STATIC_DIR="$X11_STUBS_DIR/lib_static"
mkdir -p "$X11_STATIC_DIR"
AR="$(dirname "$CLANG")/llvm-ar"

"$CLANG" --target=aarch64-linux-android28 --sysroot="$SYSROOT" \
    -O2 -fPIC -I"$X11_INCLUDE" -c "$X11_STUBS_DIR/x11_stub.c" \
    -o "$X11_STATIC_DIR/x11_stub.o"
"$AR" rcs "$X11_STATIC_DIR/libX11.a" "$X11_STATIC_DIR/x11_stub.o"
echo "Static archive created: $(ls -la "$X11_STATIC_DIR/libX11.a")"

echo ""
echo "=== Step 2: Configuring GL4ES with CMake ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

TOOLCHAIN_FILE="$BUILD_DIR/toolchain.cmake"
cat > "$TOOLCHAIN_FILE" << 'TCEOF'
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER_TARGET aarch64-linux-android28)
set(CMAKE_C_FLAGS_INIT "--target=aarch64-linux-android28 -fPIC")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "--target=aarch64-linux-android28 --rtlib=compiler-rt --unwindlib=none")
set(CMAKE_EXE_LINKER_FLAGS_INIT "--target=aarch64-linux-android28 --rtlib=compiler-rt --unwindlib=none")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
TCEOF

cd "$BUILD_DIR"

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
    -Dlog-lib="$SYSROOT/usr/lib/aarch64-linux-android/28/liblog.so" \
    -DCMAKE_C_FLAGS="-O2 -fPIC --target=aarch64-linux-android28 -I$X11_INCLUDE -Wno-int-conversion -Wno-return-mismatch -Wno-incompatible-pointer-types" \
    -DCMAKE_SHARED_LINKER_FLAGS="--target=aarch64-linux-android28 --rtlib=compiler-rt --unwindlib=none -L$X11_STATIC_DIR -Wl,--whole-archive -lX11 -Wl,--no-whole-archive -Wl,-soname,libGL.so.1"

echo ""
echo "=== Step 3: Compiling GL4ES ==="
make -j"$(nproc)"

echo ""
echo "=== Step 4: Verifying build ==="
OUTPUT="$GL4ES_SRC/lib/libGL.so.1"
echo "Output file:"
ls -la "$OUTPUT"
echo ""
echo "File type:"
file "$OUTPUT"
echo ""
echo "Dynamic dependencies:"
readelf -d "$OUTPUT" | grep -E "NEEDED|SONAME"
echo ""
echo "GLX symbols:"
GLX_COUNT=$(readelf -Ws "$OUTPUT" | grep -ci "glX" || true)
echo "Found $GLX_COUNT glX* symbols"
if [ "$GLX_COUNT" -lt 5 ]; then
    echo "WARNING: Too few GLX symbols ($GLX_COUNT). GLX may not be compiled in!"
    readelf -Ws "$OUTPUT" | grep -i "glX" || true
else
    echo "GLX support OK. Key functions:"
    readelf -Ws "$OUTPUT" | grep -iE "glXChooseVisual|glXGetFBConfig|glXCreateContext|glXMakeCurrent|glXSwapBuffers"
fi
echo ""
echo "X11 stub symbols:"
X11_SYM_COUNT=$(readelf -Ws "$OUTPUT" | grep -cE "XOpenDisplay|XGetVisualInfo|XCreateColormap" || true)
echo "Found $X11_SYM_COUNT X11 stub symbols statically linked"

echo ""
echo "=== Step 5: Packaging ==="
mkdir -p "$BUILD_DIR/output/opt/gl4es/lib"
cp "$OUTPUT" "$BUILD_DIR/output/opt/gl4es/lib/libGL.so.1"

cd "$BUILD_DIR/output"
tar cf - opt/gl4es | zstd -19 -o "$BUILD_DIR/gl4es-bionic-1.1.8.tzst"
echo "Package created:"
ls -la "$BUILD_DIR/gl4es-bionic-1.1.8.tzst"

echo ""
echo "=== Step 6: Copying to Android assets ==="
mkdir -p "$WIN_ASSETS"
cp "$BUILD_DIR/gl4es-bionic-1.1.8.tzst" "$WIN_ASSETS/gl4es-bionic-1.1.8.tzst"
echo "Copied to: $WIN_ASSETS/gl4es-bionic-1.1.8.tzst"
ls -la "$WIN_ASSETS/gl4es-bionic-1.1.8.tzst"

echo ""
echo "=========================================="
echo "=== GL4ES BUILD AND DEPLOY COMPLETE ==="
echo "=========================================="
echo "Output: $BUILD_DIR/gl4es-bionic-1.1.8.tzst"
echo "Asset:  $WIN_ASSETS/gl4es-bionic-1.1.8.tzst"

