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
    entity->orientation = quat_identity();
    entity->scale = 1.0f;
    entity->mesh = mesh;
    entity->materials = materials;
    entity->casts_shadow = 1;

    return entity;
}

Mat4 entity_matrix(const Entity *entity)
{
    const float s = entity->scale;

    return m4_multiply(m4_translate(entity->position),
                       m4_multiply(quat_to_mat4(entity->orientation), m4_scale(v3(s, s, s))));
}
