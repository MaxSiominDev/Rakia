#ifndef RENDERER_H
#define RENDERER_H

#include "engine/camera.h"
#include "engine/light.h"
#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/scene.h"
#include "engine/shader.h"
#include "engine/shadow.h"

typedef struct {
    Shader mesh_shader;
    Shader depth_shader;
    Shader sky_shader;
    Shadow shadow;
    Mesh sky_quad;
    GLuint sky_panorama;
    Material plain;
    // the shadow map is fitted around this sphere every frame
    Vec3 shadow_center;
    float shadow_radius;
} Renderer;

int renderer_init(Renderer *renderer, const char *panorama_path);
void renderer_draw(Renderer *renderer, const Scene *scene, const Camera *camera, const Light *light,
                   int width, int height);

#endif
