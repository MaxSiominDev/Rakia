#include "engine/texture.h"

#include "engine/gl_ext.h"
#include "engine/image.h"

#include <stdio.h>

static GLenum internal_format(int channels, TextureKind kind)
{
    const int srgb = kind == TEXTURE_COLOR && gl_ext_present(GLEXT_TEXTURE_SRGB);

    switch (channels) {
    case 1:
        return srgb ? GL_SLUMINANCE8_EXT : GL_LUMINANCE8;
    case 2:
        return srgb ? GL_SLUMINANCE8_ALPHA8_EXT : GL_LUMINANCE8_ALPHA8;
    case 3:
        return srgb ? GL_SRGB8_EXT : GL_RGB8;
    default:
        return srgb ? GL_SRGB8_ALPHA8_EXT : GL_RGBA8;
    }
}

static GLenum pixel_format(int channels)
{
    switch (channels) {
    case 1:
        return GL_LUMINANCE;
    case 2:
        return GL_LUMINANCE_ALPHA;
    case 3:
        return GL_RGB;
    default:
        return GL_RGBA;
    }
}

GLuint texture_load(const char *path, TextureKind kind)
{
    Image image;
    const char *reason;
    GLuint id = 0;
    GLenum error;

    if (image_load(&image, path, &reason) != 0) {
        fprintf(stderr, "cannot load texture %s: %s\n", path, reason);
        return 0;
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (gl_ext_present(GLEXT_TEXTURE_FILTER_ANISOTROPIC)) {
        GLfloat max_anisotropy = 1.0f;

        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy);
    }
    if (!gl_ext_present(GLEXT_FRAMEBUFFER_OBJECT)) {
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    }

    // errors left by earlier calls would otherwise be blamed on this upload
    while (glGetError() != GL_NO_ERROR) {
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)internal_format(image.channels, kind), image.width, image.height, 0,
                 pixel_format(image.channels), GL_UNSIGNED_BYTE, image.pixels);
    image_free(&image);

    error = glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "cannot upload texture %s: GL error 0x%x\n", path, error);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &id);
        return 0;
    }

    if (gl_ext_present(GLEXT_FRAMEBUFFER_OBJECT)) {
        glGenerateMipmapEXT(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    return id;
}
