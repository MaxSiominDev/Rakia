#ifndef WATER_H
#define WATER_H

#include "engine/camera.h"
#include "engine/gl_compat.h"
#include "engine/light.h"
#include "engine/mesh.h"
#include "engine/shader.h"
#include "engine/vecmath.h"

typedef struct {
    Shader shader;
    Mesh plane;
    GLuint ripples;
    GLuint panorama;
} Water;

int water_init(Water *water, GLuint panorama);
// the plane follows the camera, so it always reaches past the fog
void water_draw(Water *water, Mat4 view_projection, const Camera *camera, const Light *light, float time);

#endif
