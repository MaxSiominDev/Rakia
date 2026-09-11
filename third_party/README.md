# Third-party code

stb_image.h, version 2.30, public domain (or MIT, at your option). Downloaded unchanged
from https://raw.githubusercontent.com/nothings/stb/master/stb_image.h on 2026-09-11.
Compiled once in src/engine/image.c, which defines STB_IMAGE_IMPLEMENTATION.

freeglut-win32/ holds the static freeglut 3.6.0 for the Windows cross-build. It is not
committed; tools/build-freeglut-win32.sh downloads the release and builds it into that
directory with the mingw toolchain.
