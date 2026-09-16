#include "engine/texture.h"

#include "engine/gl_ext.h"
#include "engine/image.h"

#include <stdio.h>

// reaches past the fringe that mipmaps blend in, without flooding large empty margins
#define CUTOUT_BLEED_PASSES 6

static GLenum internal_format(int channels, TextureKind kind)
{
    const int srgb = kind != TEXTURE_DATA;

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

static GLuint new_texture(GLenum min_filter, GLenum wrap_s, GLenum wrap_t)
{
    GLuint id = 0;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)wrap_t);
    // errors left by earlier calls would otherwise be blamed on this upload
    while (glGetError() != GL_NO_ERROR) {
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    return id;
}

static GLuint finish_texture(GLuint id, const char *path, GLenum error)
{
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "cannot upload texture %s: GL error 0x%x\n", path, error);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &id);
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    return id;
}

GLuint texture_create(Image *image, TextureKind kind, const char *name)
{
    // screen textures draw at about one texel per pixel, and sprite sheets stay large for their short life
    const int no_mips = kind == TEXTURE_SCREEN || kind == TEXTURE_SPRITE;
    const GLenum wrap = no_mips || kind == TEXTURE_CUTOUT ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    const GLenum min_filter = no_mips ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR;
    GLuint id = new_texture(min_filter, wrap, wrap);
    GLenum error;

    // bled before the mipmaps are built, so cutout edges do not blend toward black
    if (kind == TEXTURE_CUTOUT && image->channels == 4) {
        image_bleed(image, CUTOUT_BLEED_PASSES);
    }
    if (!no_mips && gl_ext_present(GLEXT_TEXTURE_FILTER_ANISOTROPIC)) {
        GLfloat max_anisotropy = 1.0f;

        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)internal_format(image->channels, kind), image->width, image->height, 0,
                 pixel_format(image->channels), GL_UNSIGNED_BYTE, image->pixels);
    error = glGetError();
    if (error == GL_NO_ERROR && !no_mips) {
        glGenerateMipmapEXT(GL_TEXTURE_2D);
        error = glGetError();
    }

    return finish_texture(id, name, error);
}

GLuint texture_load(const char *path, TextureKind kind)
{
    Image image;
    const char *reason;
    GLuint id;

    if (image_load(&image, path, &reason) != 0) {
        fprintf(stderr, "cannot load texture %s: %s\n", path, reason);
        return 0;
    }
    id = texture_create(&image, kind, path);
    image_free(&image);

    return id;
}

GLuint texture_load_hdr(const char *path)
{
    HdrImage image;
    const char *reason;
    GLuint id;
    GLenum error;

    if (image_load_hdr(&image, path, &reason) != 0) {
        fprintf(stderr, "cannot load panorama %s: %s\n", path, reason);
        return 0;
    }

    id = new_texture(GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, image.width, image.height, 0, GL_RGB, GL_FLOAT, image.pixels);
    image_free_hdr(&image);
    error = glGetError();
    if (error == GL_NO_ERROR) {
        glGenerateMipmapEXT(GL_TEXTURE_2D);
        error = glGetError();
    }

    return finish_texture(id, path, error);
}
