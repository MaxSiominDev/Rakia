#ifndef TEXTURE_H
#define TEXTURE_H

#include "engine/gl_compat.h"
#include "engine/image.h"

// color textures are stored as sRGB so the shader samples linear values; data maps stay linear
typedef enum {
    TEXTURE_COLOR,
    TEXTURE_DATA,
    // a cutout is color that does not tile: its edges clamp instead of filtering into the far side
    TEXTURE_CUTOUT,
    // a screen texture is drawn at about one texel per pixel, where a mip level would only blur it
    TEXTURE_SCREEN,
    TEXTURE_SPRITE
} TextureKind;

GLuint texture_load(const char *relative, TextureKind kind);
// a cutout image is bled in place, so the caller's pixels change
GLuint texture_create(Image *image, TextureKind kind, const char *name);
GLuint texture_load_hdr(const char *relative);

#endif
