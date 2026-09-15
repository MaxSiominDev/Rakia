#include "engine/renderer.h"

#include "engine/gl_ext.h"
#include "engine/texture.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SHADOW_UNIT 4
#define OPAQUE_ALPHA_CUTOFF 0.5f

static int create_sky_quad(Mesh *mesh)
{
    static const float corners[4][2] = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};
    MeshVertex vertices[4];
    unsigned int indices[6] = {0, 1, 2, 0, 2, 3};
    MeshGroup group = {"", 0, 0, 6};
    MeshData data;
    int i;

    memset(vertices, 0, sizeof vertices);
    for (i = 0; i < 4; i++) {
        vertices[i].position = v3(corners[i][0], corners[i][1], 0.0f);
    }
    data.vertices = vertices;
    data.indices = indices;
    data.groups = &group;
    data.vertex_count = 4;
    data.index_count = 6;
    data.group_count = 1;

    return mesh_create(mesh, &data);
}

int renderer_init(Renderer *renderer, const char *panorama_path)
{
    if (shader_load(&renderer->mesh_shader, "mesh") != 0 || shader_load(&renderer->depth_shader, "depth") != 0 ||
        shader_load(&renderer->sky_shader, "sky") != 0) {
        return -1;
    }
    glUseProgram(renderer->mesh_shader.program);
    shader_set_int(&renderer->mesh_shader, "u_diffuse_map", MATERIAL_UNIT_DIFFUSE);
    shader_set_int(&renderer->mesh_shader, "u_normal_map", MATERIAL_UNIT_NORMAL);
    shader_set_int(&renderer->mesh_shader, "u_metal_rough_map", MATERIAL_UNIT_METAL_ROUGH);
    shader_set_int(&renderer->mesh_shader, "u_emissive_map", MATERIAL_UNIT_EMISSIVE);
    shader_set_int(&renderer->mesh_shader, "u_shadow_map", SHADOW_UNIT);
    shader_set_float(&renderer->mesh_shader, "u_shadow_texel", SHADOW_TAP_SPREAD / SHADOW_MAP_SIZE);
    glUseProgram(renderer->depth_shader.program);
    shader_set_int(&renderer->depth_shader, "u_diffuse_map", MATERIAL_UNIT_DIFFUSE);
    glUseProgram(renderer->sky_shader.program);
    shader_set_int(&renderer->sky_shader, "u_panorama", 0);
    glUseProgram(0);

    material_init();
    material_default(&renderer->plain);

    if (shadow_init(&renderer->shadow) != 0) {
        fprintf(stderr, "the depth-only framebuffer for the shadow map is incomplete\n");
        return -1;
    }
    renderer->shadow_center = v3(0.0f, 0.0f, 0.0f);
    renderer->shadow_radius = 1.0f;
    renderer->shadow_relief = 1.0f;
    renderer->shadow_reach = 1.0f;

    renderer->sky_panorama = texture_load_hdr(panorama_path);
    if (renderer->sky_panorama == 0 || create_sky_quad(&renderer->sky_quad) != 0) {
        return -1;
    }

    if (terrain_init(&renderer->terrain) != 0 || foliage_init(&renderer->foliage) != 0 ||
        water_init(&renderer->water, renderer->sky_panorama) != 0) {
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    return 0;
}

static const Material *group_material(const Renderer *renderer, const Entity *entity, const MeshGroup *group)
{
    return group->material >= 0 ? &entity->materials[group->material] : &renderer->plain;
}

// a hinged group carries a transform of its own inside the model, which is how the canopy swings open
static Mat4 group_matrix(const Entity *entity, Mat4 model, unsigned char flags)
{
    return flags & GROUP_HINGED ? m4_multiply(model, entity->hinge) : model;
}

static void draw_shadow_casters(Renderer *renderer, const Scene *scene, const Camera *camera,
                                const Light *light, int width, int height)
{
    Shader *shader = &renderer->depth_shader;
    int i;

    shadow_begin(&renderer->shadow);
    glUseProgram(shader->program);
    shader_set_mat4(shader, "u_view_projection", renderer->shadow.view_projection);

    for (i = 0; i < scene->entity_count; i++) {
        const Entity *entity = &scene->entities[i];
        Mat4 model;
        int g;

        if (!entity->casts_shadow) {
            continue;
        }
        model = entity_matrix(entity);
        mesh_bind(entity->mesh);
        for (g = 0; g < entity->mesh->group_count; g++) {
            const unsigned char flags = entity->group_flags[g];
            const Material *material = group_material(renderer, entity, &entity->mesh->groups[g]);

            if (flags & (GROUP_HIDDEN | GROUP_BLENDED)) {
                continue;
            }
            shader_set_mat4(shader, "u_model", group_matrix(entity, model, flags));
            glBindTexture(GL_TEXTURE_2D, material->diffuse_map);
            shader_set_float(shader, "u_opacity", material->opacity);
            shader_set_float(shader, "u_alpha_cutoff", material->alpha_test ? OPAQUE_ALPHA_CUTOFF : 0.0f);
            mesh_draw_group(entity->mesh, g);
        }
        mesh_unbind();
    }

    glUseProgram(0);

    terrain_draw_depth(&renderer->terrain, shader, renderer->shadow.view_projection);
    foliage_draw_depth(&renderer->foliage, renderer->shadow.view_projection, camera->eye, light->sun_direction);
    shadow_end(width, height);
}

// columns: world-space steps for one unit of screen x and y at unit depth, and the view direction, all turned
// into the panorama's frame so the shader can sample it straight from the ray
static void sky_ray_basis(const Camera *camera, const Light *light, int width, int height, float out[9])
{
    const float half_height = tanf(camera->fov_y_radians * 0.5f);
    const float half_width = half_height * (float)width / (float)height;
    const Vec3 forward = v3_normalize(v3_sub(camera->target, camera->eye));
    const Vec3 right = v3_normalize(v3_cross(forward, camera->up));
    const Vec3 up = v3_cross(right, forward);
    const Mat4 turn = m4_rotate(light->sky_yaw, v3(0.0f, 1.0f, 0.0f));
    const Vec3 columns[3] = {
        m4_transform_direction(turn, v3_scale(right, half_width)),
        m4_transform_direction(turn, v3_scale(up, half_height)),
        m4_transform_direction(turn, forward)
    };
    int i;

    for (i = 0; i < 3; i++) {
        out[i * 3] = columns[i].x;
        out[i * 3 + 1] = columns[i].y;
        out[i * 3 + 2] = columns[i].z;
    }
}

static void draw_sky(Renderer *renderer, const Camera *camera, const Light *light, int width, int height)
{
    Shader *shader = &renderer->sky_shader;
    float basis[9];

    sky_ray_basis(camera, light, width, height, basis);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(shader->program);
    glUniformMatrix3fv(shader_uniform(shader, "u_ray_basis"), 1, GL_FALSE, basis);
    shader_set_vec3(shader, "u_fog_color", light->fog_color);
    shader_set_float(shader, "u_exposure", light->exposure);
    glBindTexture(GL_TEXTURE_2D, renderer->sky_panorama);
    mesh_bind(&renderer->sky_quad);
    mesh_draw_group(&renderer->sky_quad, 0);
    mesh_unbind();
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

static void draw_groups(Renderer *renderer, const Scene *scene, int blended)
{
    Shader *shader = &renderer->mesh_shader;
    int i;

    for (i = 0; i < scene->entity_count; i++) {
        const Entity *entity = &scene->entities[i];
        Mat4 model;
        int g;

        if (entity->hidden) {
            continue;
        }
        model = entity_matrix(entity);
        shader_set_vec3(shader, "u_highlight_color", entity->highlight_color);
        shader_set_float(shader, "u_highlight", entity->highlight);
        mesh_bind(entity->mesh);
        for (g = 0; g < entity->mesh->group_count; g++) {
            const unsigned char flags = entity->group_flags[g];
            const Material *material = group_material(renderer, entity, &entity->mesh->groups[g]);

            if ((flags & GROUP_HIDDEN) || ((flags & GROUP_BLENDED) != 0) != blended) {
                continue;
            }
            shader_set_mat4(shader, "u_model", group_matrix(entity, model, flags));
            material_bind(material, shader);
            shader_set_float(shader, "u_alpha_cutoff", !blended && material->alpha_test ? OPAQUE_ALPHA_CUTOFF : 0.0f);
            shader_set_int(shader, "u_double_sided", (flags & GROUP_DOUBLE_SIDED) != 0);
            if (!(flags & GROUP_DOUBLE_SIDED)) {
                mesh_draw_group(entity->mesh, g);
            } else if (blended) {
                // inner faces first, so a glass shell composes back to front whatever its triangle order
                glCullFace(GL_FRONT);
                mesh_draw_group(entity->mesh, g);
                glCullFace(GL_BACK);
                mesh_draw_group(entity->mesh, g);
            } else {
                glDisable(GL_CULL_FACE);
                mesh_draw_group(entity->mesh, g);
                glEnable(GL_CULL_FACE);
            }
        }
        mesh_unbind();
    }
}

static void draw_meshes(Renderer *renderer, const Scene *scene, const Camera *camera, const Light *light,
                        Mat4 view_projection, int blended)
{
    Shader *shader = &renderer->mesh_shader;
    int unit;

    glUseProgram(shader->program);
    shader_set_mat4(shader, "u_view_projection", view_projection);
    shader_set_mat4(shader, "u_shadow_matrix", renderer->shadow.texture_matrix);
    shader_set_vec3(shader, "u_camera_position", camera->eye);
    light_apply(light, shader);
    glActiveTexture(GL_TEXTURE0 + SHADOW_UNIT);
    glBindTexture(GL_TEXTURE_2D, renderer->shadow.texture);
    glActiveTexture(GL_TEXTURE0);

    if (blended) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    }
    draw_groups(renderer, scene, blended);
    if (blended) {
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    for (unit = SHADOW_UNIT; unit >= 0; unit--) {
        glActiveTexture(GL_TEXTURE0 + (GLenum)unit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glUseProgram(0);
}

void renderer_draw(Renderer *renderer, const Scene *scene, const Camera *camera, const Light *light,
                   float time, int width, int height)
{
    const Mat4 view_projection = m4_multiply(camera_projection(camera, width, height), camera_view(camera));

    terrain_update(&renderer->terrain, camera->eye);
    foliage_update(&renderer->foliage, camera->eye);

    shadow_fit(&renderer->shadow, renderer->shadow_center, renderer->shadow_radius, renderer->shadow_relief,
               renderer->shadow_reach, light->sun_direction);
    draw_shadow_casters(renderer, scene, camera, light, width, height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_sky(renderer, camera, light, width, height);
    terrain_draw(&renderer->terrain, view_projection, camera, light, &renderer->shadow);
    draw_meshes(renderer, scene, camera, light, view_projection, 0);
    foliage_draw(&renderer->foliage, view_projection, camera, light, &renderer->shadow);
    water_draw(&renderer->water, view_projection, camera, light, time);
    // blended groups come last so they compose over the sea and the plants behind them
    draw_meshes(renderer, scene, camera, light, view_projection, 1);
}
