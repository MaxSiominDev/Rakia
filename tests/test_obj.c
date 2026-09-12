#include "check.h"
#include "engine/obj.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define F16_PATH "assets/raw/aircraft/f16_rickslash/f16_rickslash.obj"

static int indices_in_range(const Model *model)
{
    int i;

    for (i = 0; i < model->data.index_count; i++) {
        if (model->data.indices[i] >= (unsigned int)model->data.vertex_count) {
            return 0;
        }
    }

    return 1;
}

static int groups_tile_the_indices(const Model *model)
{
    int next = 0;
    int i;

    for (i = 0; i < model->data.group_count; i++) {
        const MeshGroup *group = &model->data.groups[i];

        if (group->first_index != next || group->index_count <= 0) {
            return 0;
        }
        next += group->index_count;
    }

    return next == model->data.index_count;
}

static int frame_is_orthonormal(const MeshVertex *vertex, float tolerance)
{
    return fabsf(v3_length(vertex->normal) - 1.0f) <= tolerance &&
           fabsf(v3_length(vertex->tangent) - 1.0f) <= tolerance &&
           fabsf(v3_dot(vertex->normal, vertex->tangent)) <= tolerance &&
           (vertex->tangent_sign == 1.0f || vertex->tangent_sign == -1.0f);
}

static const MeshVertex *group_vertex(const Model *model, int group, int corner)
{
    const int first = model->data.groups[group].first_index;

    return &model->data.vertices[model->data.indices[first + corner]];
}

static const MeshVertex *vertex_at(const Model *model, float x, float y, float z)
{
    static const MeshVertex none = {0};
    int i;

    for (i = 0; i < model->data.vertex_count; i++) {
        const MeshVertex *vertex = &model->data.vertices[i];

        if (vertex->position.x == x && vertex->position.y == y && vertex->position.z == z) {
            return vertex;
        }
    }
    check(0, "a vertex at the expected position exists");

    return &none;
}

static int material_index(const Model *model, const char *name)
{
    int i;

    for (i = 0; i < model->material_count; i++) {
        if (strcmp(model->materials[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

static int group_index(const Model *model, const char *name)
{
    int i;

    for (i = 0; i < model->data.group_count; i++) {
        if (strcmp(model->data.groups[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

static void test_shapes(void)
{
    Model model;
    const ObjMaterial *painted;
    const ObjMaterial *plain;
    const ObjMaterial *backed;

    check(obj_load(&model, "tests/fixtures/shapes.obj") == 0, "shapes.obj loads");
    check(model.data.vertex_count == 7, "corners shared by the quad and the pentagon are deduplicated");
    check(model.data.index_count == 15, "a quad gives two triangles and a pentagon three");
    check(indices_in_range(&model), "shapes indices are in range");
    check(model.data.group_count == 2, "o and s lines do not start groups");
    check(strcmp(model.data.groups[0].name, "quad") == 0 && model.data.groups[0].index_count == 6, "quad group");
    check(strcmp(model.data.groups[1].name, "pentagon") == 0 && model.data.groups[1].first_index == 6 &&
          model.data.groups[1].index_count == 9, "pentagon group follows the quad");
    check(strcmp(model.directory, "tests/fixtures") == 0, "directory is taken from the obj path");

    check(model.material_count == 3, "an mtllib name with a space is resolved from the obj directory");
    if (model.material_count != 3) {
        obj_free(&model);
        return;
    }
    painted = &model.materials[0];
    plain = &model.materials[1];
    backed = &model.materials[2];
    check(strcmp(painted->name, "painted") == 0 && strcmp(plain->name, "plain") == 0 &&
          strcmp(backed->name, "backed") == 0, "materials keep their order and names");
    check(model.data.groups[0].material == 0 && model.data.groups[1].material == 1, "groups point at their material");

    check_v3(painted->kd, 0.2f, 0.4f, 0.6f, "Kd");
    check_close(painted->opacity, 0.75f, 1e-6f, "d");
    check_v3(painted->ke, 1.0f, 0.5f, 0.25f, "Ke");
    check_close(painted->metallic, 0.3f, 1e-6f, "Pm");
    check_close(painted->roughness, 0.8f, 1e-6f, "Pr");
    check(strcmp(painted->diffuse_map, "painted diffuse.png") == 0, "map_Kd keeps a space in the file name");
    check(strcmp(painted->alpha_map, "painted alpha.png") == 0, "map_d");
    check(strcmp(painted->normal_map, "painted normal.jpg") == 0, "norm after map_Bump wins");
    check(strcmp(painted->metal_rough_map, "painted mr.jpg") == 0, "the trailing comment is stripped from map_Pr");
    check(strcmp(painted->emissive_map, "painted emissive.jpg") == 0, "map_Ke");
    check(painted->blend == 1 && painted->double_sided == 1, "alphaMode BLEND doubleSided sets both flags");

    check_v3(plain->kd, 1.0f, 1.0f, 1.0f, "a material with only map_Kd keeps Kd 1 1 1");
    check_close(plain->opacity, 1.0f, 1e-6f, "d 0 reads as opaque");
    check(strcmp(plain->diffuse_map, "plain.png") == 0 && plain->normal_map[0] == '\0' &&
          plain->metal_rough_map[0] == '\0', "maps not named stay empty");
    check(plain->blend == 0 && plain->double_sided == 0 && plain->metallic == 0.0f && plain->roughness == 1.0f,
          "flags and factors default off");
    check(backed->blend == 0 && backed->double_sided == 1, "alphaMode OPAQUE doubleSided sets only double sided");

    obj_free(&model);
}

static void test_face_forms_and_groups(void)
{
    Model model;
    const MeshVertex *vertex;
    int a;
    int b;

    check(obj_load(&model, "tests/fixtures/forms.obj") == 0, "forms.obj loads");
    check(model.data.vertex_count == 14, "the (v, vt, vn) triple decides which corners share a vertex");
    check(model.data.index_count == 18, "six triangles");
    check(indices_in_range(&model), "forms indices are in range");
    check(groups_tile_the_indices(&model), "groups cover the indices without gaps");
    check(model.data.group_count == 4, "empty groups are dropped");
    if (model.data.group_count != 4) {
        obj_free(&model);
        return;
    }

    a = material_index(&model, "a");
    b = material_index(&model, "b");
    check(a == 0 && b == 1, "forms.mtl has a and b");
    check(model.data.groups[0].name[0] == '\0' && model.data.groups[0].material == -1 &&
          model.data.groups[0].index_count == 3, "faces before the first g line form the unnamed group");
    check(strcmp(model.data.groups[1].name, "first") == 0 && model.data.groups[1].material == a &&
          model.data.groups[1].index_count == 9, "usemtl repeated for the same material keeps the group whole");
    check(strcmp(model.data.groups[2].name, "first") == 0 && model.data.groups[2].material == b &&
          model.data.groups[2].index_count == 3, "usemtl of another material splits the group under its name");
    check(strcmp(model.data.groups[3].name, "last") == 0 && model.data.groups[3].material == -1,
          "an unknown material gives -1");

    vertex = group_vertex(&model, 0, 2);
    check_v3(vertex->position, 1.0f, 1.0f, 0.0f, "v form position");
    check_v3(vertex->normal, 0.0f, 0.0f, 1.0f, "v form gets a computed normal");
    check(vertex->u == 0.0f && vertex->v == 0.0f, "v form has uv 0");
    check(frame_is_orthonormal(vertex, 1e-6f), "v form gets a fallback tangent frame");

    vertex = group_vertex(&model, 1, 1);
    check_v3(vertex->position, 1.0f, 0.0f, 0.0f, "v/vt form position");
    check(vertex->u == 1.0f && vertex->v == 0.0f, "v/vt form reads the uv");
    check_v3(vertex->tangent, 1.0f, 0.0f, 0.0f, "v/vt form tangent follows +u");

    check_v3(group_vertex(&model, 1, 3)->position, 0.0f, 0.0f, 0.0f, "-4 counts back to the first vertex");
    check_v3(group_vertex(&model, 1, 4)->position, 1.0f, 1.0f, 0.0f, "-2 counts back to the third vertex");
    vertex = group_vertex(&model, 1, 5);
    check_v3(vertex->position, 0.0f, 1.0f, 0.0f, "-1 is the last vertex");
    check_v3(vertex->normal, 0.0f, 0.0f, 1.0f, "v//vn form reads the normal");
    check(vertex->u == 0.0f && vertex->v == 0.0f, "v//vn form has uv 0");
    check(frame_is_orthonormal(vertex, 1e-6f), "v//vn form gets a fallback tangent frame");

    vertex = group_vertex(&model, 1, 7);
    check_v3(vertex->position, 1.0f, 1.0f, 0.0f, "v/vt/vn form position");
    check(vertex->u == 1.0f && vertex->v == 1.0f, "v/vt/vn form reads the uv");
    check_v3(vertex->normal, 0.0f, 0.0f, 1.0f, "v/vt/vn form reads the normal");
    check(group_vertex(&model, 1, 6) == group_vertex(&model, 2, 0), "the same triple in two groups is one vertex");

    obj_free(&model);
}

static void test_computed_normals(void)
{
    Model model;
    const float weighted = 1.0f / sqrtf(17.0f);

    check(obj_load(&model, "tests/fixtures/no_normals.obj") == 0, "no_normals.obj loads");
    check(model.data.vertex_count == 9 && model.data.index_count == 12, "no_normals counts");
    check_v3(vertex_at(&model, 2.0f, 0.0f, 0.0f)->normal, 0.0f, 0.0f, 1.0f, "a corner of the big triangle faces +z");
    check_v3(vertex_at(&model, 0.0f, 0.0f, 1.0f)->normal, 0.0f, 1.0f, 0.0f, "a corner of the small triangle faces +y");
    check_v3(vertex_at(&model, 0.0f, 0.0f, 0.0f)->normal, 0.0f, weighted, 4.0f * weighted,
             "the shared corner leans toward the bigger triangle");
    check_v3(vertex_at(&model, 4.0f, 1.0f, 0.0f)->normal, 0.0f, 0.0f, 1.0f, "a flat quad gets its plane normal");
    check(frame_is_orthonormal(vertex_at(&model, 0.0f, 0.0f, 0.0f), 1e-6f), "computed normals get a tangent frame");
    obj_free(&model);
}

static void test_tangents(void)
{
    Model model;
    int corner;

    check(obj_load(&model, "tests/fixtures/tangents.obj") == 0, "tangents.obj loads");
    check(model.data.group_count == 3, "tangents.obj has three groups");
    if (model.data.group_count != 3) {
        obj_free(&model);
        return;
    }

    for (corner = 0; corner < 6; corner++) {
        const MeshVertex *regular = group_vertex(&model, 0, corner);
        const MeshVertex *mirrored = group_vertex(&model, 1, corner);

        check(frame_is_orthonormal(regular, 1e-6f), "regular quad frame is orthonormal");
        check_v3(regular->tangent, 1.0f, 0.0f, 0.0f, "regular quad tangent points along +u");
        check(regular->tangent_sign == 1.0f, "regular quad is right-handed");
        check(frame_is_orthonormal(mirrored, 1e-6f), "mirrored quad frame is orthonormal");
        check_v3(mirrored->tangent, -1.0f, 0.0f, 0.0f, "mirrored quad tangent follows the flipped u");
        check(mirrored->tangent_sign == -1.0f, "mirrored quad is left-handed");
    }
    for (corner = 0; corner < 3; corner++) {
        const MeshVertex *degenerate = group_vertex(&model, 2, corner);

        check(frame_is_orthonormal(degenerate, 1e-6f), "coinciding uvs get a fallback tangent frame");
        check(degenerate->tangent_sign == 1.0f, "the fallback frame is right-handed");
    }

    obj_free(&model);
}

static void test_missing_mtl(void)
{
    Model model;

    check(obj_load(&model, "tests/fixtures/missing_mtl.obj") == 0, "a missing mtllib is not fatal");
    check(model.material_count == 0 && model.materials == NULL, "a missing mtllib leaves no materials");
    check(model.data.group_count == 1 && model.data.groups[0].material == -1, "usemtl without a library gives -1");
    obj_free(&model);
}

static void test_errors(void)
{
    Model model;

    check(obj_load(&model, "tests/fixtures/does_not_exist.obj") == -1, "a missing file fails");
    check(obj_load(&model, "tests/fixtures/bad_index.obj") == -1, "a face index past the last vertex fails");
    check(obj_load(&model, "tests/fixtures/bad_mtl.obj") == -1, "a bad number in the mtl fails the load");
    check(model.data.vertices == NULL && model.data.vertex_count == 0 && model.materials == NULL,
          "a failed load leaves an empty model");
}

static void test_f16(void)
{
    Model model;
    const ObjMaterial *material;
    Vec3 min;
    Vec3 max;
    int bad_frames = 0;
    int i;

    check(obj_load(&model, F16_PATH) == 0, "the F-16 loads");
    if (model.data.vertex_count == 0) {
        return;
    }
    check(model.data.index_count == 15290 * 3, "the F-16 has 15290 triangles");
    check(model.data.vertex_count == 11600, "every F-16 corner is a v/v/v triple, so vertices match the v lines");
    check(model.data.group_count == 76, "the F-16 has 76 groups");
    check(indices_in_range(&model), "F-16 indices are in range");
    check(groups_tile_the_indices(&model), "F-16 groups cover the indices without gaps");
    check(group_index(&model, "cab_keep") >= 0 && group_index(&model, "cab_around") >= 0, "the canopy groups exist");
    check(model.material_count == 1, "the F-16 has one material");
    if (model.material_count == 1) {
        material = &model.materials[0];
        check(strcmp(material->name, "lambert2") == 0, "the material is lambert2");
        check(strcmp(material->diffuse_map, "f16_rickslash_diffuse_0.png") == 0 &&
              strcmp(material->normal_map, "f16_rickslash_normal_2.jpg") == 0 &&
              strcmp(material->metal_rough_map, "f16_rickslash_metalRough_1.jpg") == 0,
              "the F-16 maps are read, the map_Pr comment stripped");
        check(material->blend == 1 && material->double_sided == 1, "the converter comment flags the F-16 material");
        check(model.data.groups[0].material == 0 && model.data.groups[75].material == 0, "every group uses it");
    }

    min = model.data.vertices[0].position;
    max = min;
    for (i = 0; i < model.data.vertex_count; i++) {
        const MeshVertex *vertex = &model.data.vertices[i];

        min = v3(fminf(min.x, vertex->position.x), fminf(min.y, vertex->position.y), fminf(min.z, vertex->position.z));
        max = v3(fmaxf(max.x, vertex->position.x), fmaxf(max.y, vertex->position.y), fmaxf(max.z, vertex->position.z));
        if (!frame_is_orthonormal(vertex, 1e-3f)) {
            bad_frames++;
        }
    }
    check_close(max.x - min.x, 1.244f, 0.01f, "F-16 width");
    check_close(max.y - min.y, 0.607f, 0.01f, "F-16 height");
    check_close(max.z - min.z, 2.0f, 0.01f, "F-16 length");
    check_close(min.y, 0.0f, 0.01f, "the F-16 stands on y = 0");
    check_close(min.x + max.x, 0.0f, 0.01f, "the F-16 is centered on x");
    check(bad_frames == 0, "every F-16 vertex has a unit normal and a unit tangent at right angles");

    obj_free(&model);
}

static void test_free(void)
{
    Model model;

    memset(&model, 0, sizeof model);
    obj_free(&model);
    check(model.data.vertices == NULL && model.materials == NULL, "freeing an empty model is harmless");

    check(obj_load(&model, "tests/fixtures/shapes.obj") == 0, "shapes.obj loads again");
    obj_free(&model);
    check(model.data.vertices == NULL && model.data.indices == NULL && model.data.groups == NULL &&
          model.materials == NULL && model.data.vertex_count == 0 && model.material_count == 0 &&
          model.directory[0] == '\0', "free clears the model");
}

void test_obj_main(void)
{
    test_shapes();
    test_face_forms_and_groups();
    test_computed_normals();
    test_tangents();
    test_missing_mtl();
    test_errors();
    test_f16();
    test_free();
}
