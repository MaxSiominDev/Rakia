#!/bin/sh
set -eu

VERSION=3.6.0
ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d)
SYSROOT=$(cd "$(dirname "$(x86_64-w64-mingw32-gcc -print-file-name=libopengl32.a)")/.." && pwd -P)
# Homebrew prefixes the resource compiler with the target triplet, MSYS2 does not.
if command -v x86_64-w64-mingw32-windres > /dev/null 2>&1; then
    WINDRES=x86_64-w64-mingw32-windres
else
    WINDRES=windres
fi

trap 'rm -rf "$WORK"' EXIT

curl -sSL -o "$WORK/freeglut.tar.gz" \
    "https://github.com/freeglut/freeglut/releases/download/v$VERSION/freeglut-$VERSION.tar.gz"
tar xzf "$WORK/freeglut.tar.gz" -C "$WORK"

cat > "$WORK/toolchain.cmake" <<TOOLCHAIN
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER $WINDRES)
set(CMAKE_FIND_ROOT_PATH $SYSROOT)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
TOOLCHAIN

cmake -S "$WORK/freeglut-$VERSION" -B "$WORK/build" -G "Unix Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE="$WORK/toolchain.cmake" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$ROOT/third_party/freeglut-win32" \
    -DFREEGLUT_BUILD_SHARED_LIBS=OFF \
    -DFREEGLUT_BUILD_STATIC_LIBS=ON \
    -DFREEGLUT_BUILD_DEMOS=OFF

cmake --build "$WORK/build" -j"$(getconf _NPROCESSORS_ONLN)"
cmake --install "$WORK/build"

rm -rf "$ROOT/third_party/freeglut-win32/lib/cmake" \
       "$ROOT/third_party/freeglut-win32/lib/pkgconfig"

echo "freeglut $VERSION installed into third_party/freeglut-win32"
