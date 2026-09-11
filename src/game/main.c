#include "engine/assets.h"
#include "engine/camera.h"
#include "engine/gl_ext.h"
#include "engine/input.h"
#include "engine/mesh.h"
#include "engine/mesh_data.h"
#include "engine/options.h"
#include "engine/platform.h"
#include "engine/shader.h"
#include "engine/texture.h"
#include "engine/vecmath.h"

#include <stdio.h>
#include <stdlib.h>

#define KEY_ESCAPE 27
#define CUBE_TEXTURE "raw/textures/apron_concrete_concrete_pavement/concrete_pavement_diffuse_2k.jpg"
#define CUBE_TURN_RADIANS_PER_SECOND 0.4f
#define FULL_TURN (2.0f * VEC_PI)

typedef struct {
    Shader shader;
    Mesh cube;
    GLuint texture;
    Camera camera;
    float angle;
} Demo;

static int demo_init(void *context)
{
    Demo *demo = context;
    MeshData cube;
    char texture_path[ASSETS_PATH_MAX];

    if (shader_load(&demo->shader, "mesh") != 0) {
        return -1;
    }
    if (assets_path(texture_path, sizeof texture_path, CUBE_TEXTURE) != 0) {
        fprintf(stderr, "texture path is too long: %s\n", CUBE_TEXTURE);
        return -1;
    }
    demo->texture = texture_load(texture_path, TEXTURE_COLOR);
    if (demo->texture == 0) {
        return -1;
    }
    if (mesh_data_cube(&cube) != 0) {
        fprintf(stderr, "out of memory building the cube\n");
        return -1;
    }
    mesh_create(&demo->cube, &cube);
    mesh_data_free(&cube);

    demo->camera.eye = v3(2.0f, 1.4f, 2.6f);
    demo->camera.target = v3(0.0f, 0.0f, 0.0f);
    demo->camera.up = v3(0.0f, 1.0f, 0.0f);
    demo->camera.fov_y_radians = 45.0f * VEC_PI / 180.0f;
    demo->camera.near_plane = 0.1f;
    demo->camera.far_plane = 100.0f;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.45f, 0.55f, 0.68f, 1.0f);

    return 0;
}

static void demo_update(void *context, const Input *input, float dt)
{
    Demo *demo = context;

    // until the game has a pause state, Esc simply ends the program
    if (input_key_is_down(input, KEY_ESCAPE)) {
        exit(EXIT_SUCCESS);
    }

    demo->angle += CUBE_TURN_RADIANS_PER_SECOND * dt;
    if (demo->angle > FULL_TURN) {
        demo->angle -= FULL_TURN;
    }
}

static void demo_render(void *context, int width, int height)
{
    Demo *demo = context;
    const Mat4 model = m4_rotate(demo->angle, v3(0.0f, 1.0f, 0.0f));
    const Mat4 view = camera_view(&demo->camera);
    const Mat4 projection = camera_projection(&demo->camera, width, height);
    const Vec3 to_sun = v3_normalize(v3(-0.6f, 0.35f, 0.7f));
    const Vec3 eye = demo->camera.eye;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(demo->shader.program);
    glUniformMatrix4fv(shader_uniform(&demo->shader, "u_model"), 1, GL_FALSE, model.m);
    glUniformMatrix4fv(shader_uniform(&demo->shader, "u_view"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(shader_uniform(&demo->shader, "u_projection"), 1, GL_FALSE, projection.m);
    glUniform3f(shader_uniform(&demo->shader, "u_sun_direction"), to_sun.x, to_sun.y, to_sun.z);
    glUniform3f(shader_uniform(&demo->shader, "u_sun_color"), 1.3f, 1.05f, 0.75f);
    glUniform3f(shader_uniform(&demo->shader, "u_ambient"), 0.18f, 0.2f, 0.26f);
    glUniform3f(shader_uniform(&demo->shader, "u_camera_position"), eye.x, eye.y, eye.z);
    glUniform1i(shader_uniform(&demo->shader, "u_diffuse"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, demo->texture);

    mesh_draw(&demo->cube);

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

int main(int argc, char **argv)
{
    static Demo demo;
    Options options;
    PlatformApp app;
    char error[256];

    if (options_parse(&options, argc, argv, error, sizeof error) != 0) {
        fprintf(stderr, "%s\n", error);
        options_print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    assets_init(options.assets_dir);

    app.context = &demo;
    app.init = demo_init;
    app.update = demo_update;
    app.render = demo_render;

    return platform_run(&options, &app, &argc, argv);
}
