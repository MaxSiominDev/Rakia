#include "engine/water.h"

#include "engine/assets.h"
#include "engine/gl_ext.h"
#include "engine/material.h"
#include "engine/mesh_data.h"
#include "engine/terrain.h"

#include <stdio.h>
#include <string.h>

#define RIPPLE_MAP "raw/water/sea_waves_keith333/SeaWaves_N.jpg"
#define PLANE_SIZE (TERRAIN_VIEW_DISTANCE * 3.0f)
#define RIPPLE_UNIT 0
#define PANORAMA_UNIT 1

int water_init(Water *water, GLuint panorama)
{
    char path[ASSETS_PATH_MAX];
    MeshData quad;

    memset(water, 0, sizeof *water);
    if (shader_load(&water->shader, "water") != 0) {
        return -1;
    }
    if (assets_path(path, sizeof path, RIPPLE_MAP) != 0) {
        fprintf(stderr, "asset path is too long: %s\n", RIPPLE_MAP);
        return -1;
    }
    water->panorama = panorama;
    water->ripples = material_texture(path, TEXTURE_DATA);
    if (water->ripples == 0) {
        return -1;
    }

    glUseProgram(water->shader.program);
    shader_set_int(&water->shader, "u_ripples", RIPPLE_UNIT);
    shader_set_int(&water->shader, "u_panorama", PANORAMA_UNIT);
    glUseProgram(0);

    // the shader tiles the ripples by world position, so one texture tile over the whole plane is fine
    if (mesh_data_quad(&quad, PLANE_SIZE, PLANE_SIZE) != 0 || mesh_create(&water->plane, &quad) != 0) {
        fprintf(stderr, "out of memory building the sea\n");
        return -1;
    }
    mesh_data_free(&quad);

    return 0;
}

void water_draw(Water *water, Mat4 view_projection, const Camera *camera, const Light *light, float time)
{
    Shader *shader = &water->shader;

    glUseProgram(shader->program);
    shader_set_mat4(shader, "u_view_projection", view_projection);
    shader_set_mat4(shader, "u_model", m4_translate(v3(camera->eye.x, TERRAIN_SEA_LEVEL, camera->eye.z)));
    shader_set_vec3(shader, "u_camera_position", camera->eye);
    shader_set_float(shader, "u_time", time);
    shader_set_float(shader, "u_sky_yaw", light->sky_yaw);
    light_apply(light, shader);
    glActiveTexture(GL_TEXTURE0 + PANORAMA_UNIT);
    glBindTexture(GL_TEXTURE_2D, water->panorama);
    glActiveTexture(GL_TEXTURE0 + RIPPLE_UNIT);
    glBindTexture(GL_TEXTURE_2D, water->ripples);

    mesh_bind(&water->plane);
    mesh_draw_group(&water->plane, 0);
    mesh_unbind();

    glActiveTexture(GL_TEXTURE0 + PANORAMA_UNIT);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0 + RIPPLE_UNIT);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}
