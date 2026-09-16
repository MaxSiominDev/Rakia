#include "engine/post.h"

#include "engine/gl_ext.h"

#include <stdio.h>
#include <string.h>

#define SCENE_SAMPLES 4
// a steep surface facing this low sun clears 2 on direct light alone, so the threshold sits above it
#define BLOOM_THRESHOLD 2.2f
#define BLOOM_KNEE 1.0f
#define BLOOM_HALF_STRENGTH 0.5f
#define BLOOM_QUARTER_STRENGTH 0.4f
// threshold 0 makes the bright pass a plain downsample for the quarter level
#define DOWNSAMPLE_KNEE 0.0001f

static int create_fullscreen_triangle(Mesh *mesh)
{
    static const float corners[3][2] = {{-1.0f, -1.0f}, {3.0f, -1.0f}, {-1.0f, 3.0f}};
    MeshVertex vertices[3];
    unsigned int indices[3] = {0, 1, 2};
    MeshGroup group = {"", 0, 0, 3};
    MeshData data;
    int i;

    memset(vertices, 0, sizeof vertices);
    for (i = 0; i < 3; i++) {
        vertices[i].position = v3(corners[i][0], corners[i][1], 0.0f);
    }
    data.vertices = vertices;
    data.indices = indices;
    data.groups = &group;
    data.vertex_count = 3;
    data.index_count = 3;
    data.group_count = 1;

    return mesh_create(mesh, &data);
}

int post_sample_count(int requested, int max_samples)
{
    return requested < max_samples ? requested : max_samples;
}

int post_bloom_dimension(int full, int divisor)
{
    const int scaled = full / divisor;

    return scaled > 0 ? scaled : 1;
}

int post_init(Post *post)
{
    memset(post, 0, sizeof *post);
    if (shader_load_split(&post->bright_shader, "post", "bright") != 0 ||
        shader_load_split(&post->blur_shader, "post", "blur") != 0 ||
        shader_load(&post->composite_shader, "post") != 0 ||
        create_fullscreen_triangle(&post->triangle) != 0) {
        return -1;
    }

    glUseProgram(post->bright_shader.program);
    shader_set_int(&post->bright_shader, "u_source", 0);
    glUseProgram(post->blur_shader.program);
    shader_set_int(&post->blur_shader, "u_source", 0);
    glUseProgram(post->composite_shader.program);
    shader_set_int(&post->composite_shader, "u_scene", 0);
    shader_set_int(&post->composite_shader, "u_bloom_half", 1);
    shader_set_int(&post->composite_shader, "u_bloom_quarter", 2);
    glUseProgram(0);

    // the window has no samples of its own; this is for the multisampled scene FBO
    glEnable(GL_MULTISAMPLE);

    return 0;
}

static void free_targets(Post *post)
{
    if (post->width == 0) {
        return;
    }
    glDeleteFramebuffersEXT(1, &post->scene_fbo);
    glDeleteRenderbuffersEXT(1, &post->scene_color);
    glDeleteRenderbuffersEXT(1, &post->scene_depth);
    glDeleteFramebuffersEXT(1, &post->scratch_fbo);
    glDeleteTextures(1, &post->resolve);
    glDeleteTextures(2, post->bloom[0]);
    glDeleteTextures(2, post->bloom[1]);
}

static GLuint multisample_renderbuffer(GLenum format, int width, int height, int samples)
{
    GLuint id;

    glGenRenderbuffersEXT(1, &id);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, id);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, samples, format, width, height);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

    return id;
}

static GLuint bloom_texture(int width, int height)
{
    GLuint id;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    return id;
}

static void rebuild(Post *post, int width, int height)
{
    const int half_w = post_bloom_dimension(width, 2);
    const int half_h = post_bloom_dimension(height, 2);
    const int quarter_w = post_bloom_dimension(width, 4);
    const int quarter_h = post_bloom_dimension(height, 4);
    GLint max_samples = 0;
    GLenum status;

    free_targets(post);

    glGetIntegerv(GL_MAX_SAMPLES_EXT, &max_samples);
    post->samples = post_sample_count(SCENE_SAMPLES, max_samples);
    post->scene_color = multisample_renderbuffer(GL_RGBA16F_ARB, width, height, post->samples);
    post->scene_depth = multisample_renderbuffer(GL_DEPTH_COMPONENT24, width, height, post->samples);
    glGenFramebuffersEXT(1, &post->scene_fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, post->scene_fbo);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, post->scene_color);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, post->scene_depth);
    status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
        fprintf(stderr, "the multisampled HDR scene framebuffer is incomplete\n");
    }

    post->resolve = bloom_texture(width, height);
    post->bloom[0][0] = bloom_texture(half_w, half_h);
    post->bloom[0][1] = bloom_texture(half_w, half_h);
    post->bloom[1][0] = bloom_texture(quarter_w, quarter_h);
    post->bloom[1][1] = bloom_texture(quarter_w, quarter_h);
    glGenFramebuffersEXT(1, &post->scratch_fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    post->width = width;
    post->height = height;
}

void post_begin(Post *post, int width, int height)
{
    if (post->width != width || post->height != height) {
        rebuild(post, width, height);
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, post->scene_fbo);
    glViewport(0, 0, width, height);
}

static void draw_triangle(const Post *post)
{
    mesh_bind(&post->triangle);
    mesh_draw_group(&post->triangle, 0);
    mesh_unbind();
}

static void blur_pass(Post *post, GLuint source, GLuint dest, int width, int height, float dx, float dy)
{
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, dest, 0);
    glViewport(0, 0, width, height);
    glBindTexture(GL_TEXTURE_2D, source);
    glUniform2f(shader_uniform(&post->blur_shader, "u_direction"), dx, dy);
    draw_triangle(post);
}

void post_end(Post *post, const Light *light)
{
    const int half_w = post_bloom_dimension(post->width, 2);
    const int half_h = post_bloom_dimension(post->height, 2);
    const int quarter_w = post_bloom_dimension(post->width, 4);
    const int quarter_h = post_bloom_dimension(post->height, 4);

    glDisable(GL_DEPTH_TEST);

    glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, post->scene_fbo);
    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, post->scratch_fbo);
    glFramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, post->resolve, 0);
    glBlitFramebufferEXT(0, 0, post->width, post->height, 0, 0, post->width, post->height, GL_COLOR_BUFFER_BIT,
                         GL_NEAREST);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, post->scratch_fbo);
    glActiveTexture(GL_TEXTURE0);

    glUseProgram(post->bright_shader.program);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, post->bloom[0][0], 0);
    glViewport(0, 0, half_w, half_h);
    glBindTexture(GL_TEXTURE_2D, post->resolve);
    glUniform2f(shader_uniform(&post->bright_shader, "u_texel"), 1.0f / (float)post->width,
               1.0f / (float)post->height);
    shader_set_float(&post->bright_shader, "u_threshold", BLOOM_THRESHOLD);
    shader_set_float(&post->bright_shader, "u_knee", BLOOM_KNEE);
    draw_triangle(post);

    glUseProgram(post->blur_shader.program);
    blur_pass(post, post->bloom[0][0], post->bloom[0][1], half_w, half_h, 1.0f / (float)half_w, 0.0f);
    blur_pass(post, post->bloom[0][1], post->bloom[0][0], half_w, half_h, 0.0f, 1.0f / (float)half_h);

    glUseProgram(post->bright_shader.program);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, post->bloom[1][0], 0);
    glViewport(0, 0, quarter_w, quarter_h);
    glBindTexture(GL_TEXTURE_2D, post->bloom[0][0]);
    glUniform2f(shader_uniform(&post->bright_shader, "u_texel"), 1.0f / (float)half_w, 1.0f / (float)half_h);
    shader_set_float(&post->bright_shader, "u_threshold", 0.0f);
    shader_set_float(&post->bright_shader, "u_knee", DOWNSAMPLE_KNEE);
    draw_triangle(post);

    glUseProgram(post->blur_shader.program);
    blur_pass(post, post->bloom[1][0], post->bloom[1][1], quarter_w, quarter_h, 1.0f / (float)quarter_w, 0.0f);
    blur_pass(post, post->bloom[1][1], post->bloom[1][0], quarter_w, quarter_h, 0.0f, 1.0f / (float)quarter_h);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glViewport(0, 0, post->width, post->height);
    glUseProgram(post->composite_shader.program);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, post->bloom[1][0]);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, post->bloom[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, post->resolve);
    shader_set_float(&post->composite_shader, "u_exposure", light->exposure);
    shader_set_float(&post->composite_shader, "u_bloom_half_strength", BLOOM_HALF_STRENGTH);
    shader_set_float(&post->composite_shader, "u_bloom_quarter_strength", BLOOM_QUARTER_STRENGTH);
    draw_triangle(post);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
}
