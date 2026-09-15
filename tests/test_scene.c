#include "check.h"
#include "engine/scene.h"
#include "engine/vecmath.h"

#include <string.h>

#define HALF_PI (VEC_PI * 0.5f)

static void test_add(void)
{
    static Scene scene;
    MeshGroup groups[2] = {{"body", 0, 0, 6}, {"cab_keep", 0, 6, 3}};
    Mesh mesh;
    Mesh wide;
    Entity *entity;
    int i;

    memset(&mesh, 0, sizeof mesh);
    mesh.groups = groups;
    mesh.group_count = 2;
    wide = mesh;
    wide.group_count = SCENE_MAX_GROUPS + 1;

    entity = scene_add(&scene, &mesh, NULL);
    check(entity != NULL && scene.entity_count == 1, "an entity is added");
    check(entity->scale == 1.0f && !entity->hidden && entity->casts_shadow == 1,
          "entity starts at scale 1, drawn and casting a shadow");
    check(entity->mesh == &mesh && entity->orientation.w == 1.0f && entity->highlight == 0.0f,
          "entity starts unturned and untinted");
    check(entity->group_flags[0] == 0 && entity->group_flags[1] == 0, "groups start visible and opaque");
    check(mesh_group_index(&mesh, "cab_keep") == 1, "a group is found by name");
    check(mesh_group_index(&mesh, "canopy") == -1, "an unknown group name gives -1");

    check(scene_add(&scene, &wide, NULL) == NULL, "a mesh with too many groups is refused");
    for (i = 1; i < SCENE_MAX_ENTITIES; i++) {
        scene_add(&scene, &mesh, NULL);
    }
    check(scene.entity_count == SCENE_MAX_ENTITIES, "the scene fills up");
    check(scene_add(&scene, &mesh, NULL) == NULL, "a full scene refuses another entity");
}

static void test_matrix(void)
{
    const Quat quarter_left = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), HALF_PI);
    const Quat nose_up = quat_from_axis_angle(v3(1.0f, 0.0f, 0.0f), -HALF_PI);
    Entity entity;

    memset(&entity, 0, sizeof entity);
    entity.orientation = quat_identity();
    entity.scale = 1.0f;

    check_v3(m4_transform_direction(entity_matrix(&entity), v3(0.0f, 0.0f, 1.0f)), 0.0f, 0.0f, 1.0f,
             "an unturned entity keeps the model facing +z");

    entity.orientation = quarter_left;
    check_v3(m4_transform_direction(entity_matrix(&entity), v3(0.0f, 0.0f, 1.0f)), 1.0f, 0.0f, 0.0f,
             "a quarter turn about +y takes the nose toward +x");

    entity.orientation = nose_up;
    check_v3(m4_transform_direction(entity_matrix(&entity), v3(0.0f, 0.0f, 1.0f)), 0.0f, 1.0f, 0.0f,
             "a quarter turn about the left wing raises the nose");

    entity.orientation = quat_multiply(quarter_left, nose_up);
    check_v3(m4_transform_direction(entity_matrix(&entity), v3(0.0f, 0.0f, 1.0f)), 0.0f, 1.0f, 0.0f,
             "the pitch is applied in the turned frame");

    entity.orientation = quat_identity();
    entity.position = v3(1.0f, 2.0f, 3.0f);
    entity.scale = 2.0f;
    check_v3(m4_transform_point(entity_matrix(&entity), v3(0.0f, 0.0f, 1.0f)), 1.0f, 2.0f, 5.0f,
             "scale applies before the move");
}

void test_scene_main(void)
{
    test_add();
    test_matrix();
}
