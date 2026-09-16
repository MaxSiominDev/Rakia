#ifndef RENDERER_H
#define RENDERER_H

#include "engine/camera.h"
#include "engine/foliage.h"
#include "engine/light.h"
#include "engine/material.h"
#include "engine/mesh.h"
#include "engine/particles.h"
#include "engine/scene.h"
#include "engine/shader.h"
#include "engine/shadow.h"
#include "engine/terrain.h"
#include "engine/water.h"

typedef struct {
    Shader mesh_shader;
    Shader depth_shader;
    Shader sky_shader;
    Shadow shadow;
    Mesh sky_quad;
    GLuint sky_panorama;
    Material plain;
    Terrain terrain;
    Foliage foliage;
    Water water;
    // the shadow map is fitted around this piece of ground and the casters over it every frame
    Vec3 shadow_center;
    float shadow_radius;
    float shadow_relief;
    float shadow_reach;
} Renderer;

int renderer_init(Renderer *renderer, const char *panorama_path);
// time in seconds since the start; only the sea moves with it. particles draws last, over everything else
void renderer_draw(Renderer *renderer, const Scene *scene, Particles *particles, const Camera *camera,
                   const Light *light, float time, int width, int height);

#endif
