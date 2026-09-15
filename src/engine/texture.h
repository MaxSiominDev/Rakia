#ifndef TEXTURE_H
#define TEXTURE_H

#include "engine/gl_compat.h"

// color textures are stored as sRGB so the shader samples linear values; data maps stay linear
typedef enum {
    TEXTURE_COLOR,
    TEXTURE_DATA,
    // a cutout is color that does not tile: its edges clamp instead of filtering into the far side
    TEXTURE_CUTOUT,
    // a screen texture is drawn at about one texel per pixel, where a mip level would only blur it
    TEXTURE_SCREEN
} TextureKind;

GLuint texture_load(const char *path, TextureKind kind);
// float rgb, no mipmaps, wrapping horizontally the way an equirect panorama needs
GLuint texture_load_hdr(const char *path);

#endif
