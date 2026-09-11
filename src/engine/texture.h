#ifndef TEXTURE_H
#define TEXTURE_H

#include "engine/gl_compat.h"

// color textures are stored as sRGB so the shader samples linear values; data maps stay linear
typedef enum {
    TEXTURE_COLOR,
    TEXTURE_DATA
} TextureKind;

GLuint texture_load(const char *path, TextureKind kind);

#endif
