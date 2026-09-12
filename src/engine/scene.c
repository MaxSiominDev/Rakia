#include "engine/scene.h"

#include <string.h>

Entity *scene_add(Scene *scene, const Mesh *mesh, const Material *materials)
{
    Entity *entity;

    if (scene->entity_count == SCENE_MAX_ENTITIES || mesh->group_count > SCENE_MAX_GROUPS) {
        return NULL;
    }

    entity = &scene->entities[scene->entity_count++];
    memset(entity, 0, sizeof *entity);
    entity->scale = 1.0f;
    entity->mesh = mesh;
    entity->materials = materials;
    entity->casts_shadow = 1;

    return entity;
}

Mat4 entity_matrix(const Entity *entity)
{
    // a positive turn about +x would drop the nose, hence the sign on pitch
    const Mat4 turn = m4_multiply(m4_rotate(entity->yaw, v3(0.0f, 1.0f, 0.0f)),
                                  m4_multiply(m4_rotate(-entity->pitch, v3(1.0f, 0.0f, 0.0f)),
                                              m4_rotate(entity->roll, v3(0.0f, 0.0f, 1.0f))));
    const float s = entity->scale;

    return m4_multiply(m4_translate(entity->position), m4_multiply(turn, m4_scale(v3(s, s, s))));
}
