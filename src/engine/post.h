#ifndef POST_H
#define POST_H

#include "engine/gl_compat.h"
#include "engine/light.h"
#include "engine/mesh.h"
#include "engine/shader.h"

typedef struct {
    int width;
    int height;
    int samples;
    GLuint scene_fbo;
    GLuint scene_color;
    GLuint scene_depth;
    GLuint scratch_fbo;
    GLuint resolve;
    // 0 half resolution, 1 quarter
    GLuint bloom[2][2];
    Shader bright_shader;
    Shader blur_shader;
    Shader composite_shader;
    Mesh triangle;
} Post;

int post_init(Post *post);
void post_begin(Post *post, int width, int height);
void post_end(Post *post, const Light *light);

int post_sample_count(int requested, int max_samples);
int post_bloom_dimension(int full, int divisor);

#endif
