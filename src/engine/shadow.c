#include "engine/shadow.h"

#include "engine/gl_ext.h"

#include <math.h>
#include <stddef.h>

int shadow_init(Shadow *shadow)
{
    // everything outside the map counts as lit; a receiver beyond the box shows no edge
    const GLfloat lit_border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLenum status;

    glGenTextures(1, &shadow->texture);
    glBindTexture(GL_TEXTURE_2D, shadow->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, lit_border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffersEXT(1, &shadow->framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shadow->framebuffer);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, shadow->texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);

    return status == GL_FRAMEBUFFER_COMPLETE_EXT ? 0 : -1;
}

void shadow_fit(Shadow *shadow, Vec3 center, float radius, float relief, float reach, Vec3 sun_direction)
{
    const float sine = sun_direction.y;
    const float cosine = sqrtf(1.0f - sine * sine);
    // the light's own up axis stands almost straight up under a low sun, so the ground leans along it by only
    // the sine of the elevation: fitting that axis to what the ground needs is what keeps a small shadow sharp
    const float along = radius * sine + relief * cosine;
    // a caster on the sun ray through the center shades the center itself, so only the depth has to reach it
    const float depth = radius * cosine + reach / sine;
    const Vec3 eye = v3_add(center, v3_scale(sun_direction, depth * 2.0f));
    // world up is a valid up vector for the light view as long as the sun stays this low
    const Mat4 view = m4_look_at(eye, center, v3(0.0f, 1.0f, 0.0f));
    const Mat4 projection = m4_orthographic(-radius, radius, -along, along, depth, depth * 3.0f);
    const Mat4 to_texture = m4_multiply(m4_translate(v3(0.5f, 0.5f, 0.5f)), m4_scale(v3(0.5f, 0.5f, 0.5f)));

    shadow->view_projection = m4_multiply(projection, view);
    shadow->texture_matrix = m4_multiply(to_texture, shadow->view_projection);
}

void shadow_begin(const Shadow *shadow)
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shadow->framebuffer);
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glClear(GL_DEPTH_BUFFER_BIT);
    // only back faces go into the map, so no depth bias is needed
    glCullFace(GL_FRONT);
}

void shadow_end(int width, int height)
{
    glCullFace(GL_BACK);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glViewport(0, 0, width, height);
}
