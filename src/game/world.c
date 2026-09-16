#include "game/world.h"

#include "engine/assets.h"
#include "engine/mesh_data.h"
#include "engine/obj.h"

#include <math.h>
#include <stdio.h>

#define JET_MODEL "raw/aircraft/f16_rickslash/f16_rickslash.obj"
#define JET_SCALE 7.5f
#define AIM9_MODEL "raw/weapons/aim9_rickslash/aim9_rickslash.obj"
// the collected missiles are two units long, so the factor to metres is half the real length
#define AIM9_SCALE 1.425f
#define JSOW_MODEL "raw/weapons/agm154_jsow_rickslash/agm154_jsow_rickslash.obj"
#define JSOW_SCALE 2.05f
#define SHELTER_MODEL "raw/airfield/shelter_quonset_hut/shelter_quonset_hut.obj"
// two units long as well, and the hut has to swallow a fifteen metre jet
#define SHELTER_SCALE 12.5f
// the hut stands on a floor slab of its own, which is sunk until its top lands on the apron
#define SHELTER_SINK 1.19f
#define CART_MODEL "raw/airfield/missile_cart_r27/missile_cart_r27.obj"
#define LADDER_MODEL "raw/airfield/maintenance_ladder/maintenance_ladder.obj"
#define LADDER_SCALE 1.33f

#define APRON_TEXTURE "raw/textures/apron_concrete_concrete_pavement/concrete_pavement"
#define RUNWAY_TEXTURE "raw/textures/runway_asphalt_clean_asphalt/clean_asphalt"
#define ROUNDEL_TEXTURE "raw/markings/iaf-roundel"

#define APRON_SIZE 200.0f
#define APRON_TILE 1.8f
#define APRON_Z (-1150.0f)
#define APRON_EDGE (WORLD_APRON_X - APRON_SIZE * 0.5f)
#define RUNWAY_TILE 4.0f
#define RUNWAY_EDGE (WORLD_RUNWAY_WIDTH * 0.5f)
// the strip that carries the jet from the apron edge to the runway edge, laid along the taxi path
#define TAXIWAY_WIDTH 40.0f

// centerline dashes and the piano keys at both thresholds, painted just over the asphalt
#define PAINT_LIFT 0.03f
#define DASH_LENGTH 30.0f
#define DASH_PERIOD 50.0f
#define DASH_WIDTH 0.9f
#define DASH_MARGIN 100.0f
#define DASH_SPAN (WORLD_RUNWAY_LENGTH - 2.0f * DASH_MARGIN)
#define DASH_COUNT ((int)(DASH_SPAN / DASH_PERIOD) + 1)
#define PIANO_COUNT 8
#define PIANO_LENGTH 30.0f
#define PIANO_WIDTH 1.8f
#define PIANO_INSET 6.0f
#define PAINT_RECTS (DASH_COUNT + 2 * PIANO_COUNT)

// two shelters behind the parking spot, the cart and the boarding ladder beside the jet, as offsets from it
#define SHELTER_BACK 34.0f
#define SHELTER_GAP 28.0f
#define CART_OFFSET_X (-13.0f)
#define CART_OFFSET_Z 4.0f
#define LADDER_OFFSET_X 1.3f
#define LADDER_OFFSET_Z 3.0f
// the four missiles lie across the cart in its own cradle
#define CART_LOAD_X 0.3f
#define CART_LOAD_SPACING 0.62f

#define CONE_COUNT 3
#define CONE_RADIUS 0.28f
#define CONE_HEIGHT 0.75f
#define CONE_SIDES 12

// the model carries no extended gear, only the closed doors, so the legs it stands on are built here
#define GEAR_LEGS 3
#define GEAR_TUBES (2 * GEAR_LEGS)
#define STRUT_RADIUS 0.08f
#define NOSE_STRUT_RADIUS 0.07f
#define MAIN_WHEEL_RADIUS 0.36f
#define MAIN_WHEEL_HALF_WIDTH 0.12f
#define NOSE_WHEEL_RADIUS 0.25f
#define NOSE_WHEEL_HALF_WIDTH 0.09f
// where the legs meet the belly and where they put the wheels, in the jet's own frame
#define MAIN_LEG_X 0.72f
#define MAIN_LEG_TOP 0.46f
#define MAIN_LEG_Z (-1.03f)
#define MAIN_TRACK 1.18f
#define NOSE_LEG_TOP 0.41f
#define NOSE_LEG_Z 1.49f

// the roundels sit a few centimetres proud of the skin, two on the wings and one on each side of the fuselage
#define ROUNDEL_COUNT 4
#define ROUNDEL_WING_X 2.48f
#define ROUNDEL_WING_Y 1.40f
#define ROUNDEL_WING_Z (-1.20f)
#define ROUNDEL_WING_HALF 0.60f
#define ROUNDEL_SIDE_X 1.35f
#define ROUNDEL_SIDE_Y 1.69f
#define ROUNDEL_SIDE_Z 0.75f
#define ROUNDEL_SIDE_HALF 0.34f

#define CANOPY_OPEN (40.0f * VEC_DEGREES)

static const char *const canopy_groups[2] = {"cab_keep", "cab_around"};
static const char *const pylon_groups[WORLD_PYLONS] = {"rocket_holder_jsow", "rocket_holder_jsow_2",
                                                       "rocket_holder_aim9", "rocket_holder_aim9_2"};
// the two holders at the very wing tips, whatever the model calls them
static const char *const rail_groups[2] = {"rocket_holder_aim_120", "rocket_holder_aim_120_2"};
// the R-27s the cart was modeled with, hidden so our own missiles can lie in their cradle
#define CART_LOAD_GROUP "Object_2"
#define CART_FRAME_GROUP "Object_3"

static int open_model(Model *model, const char *relative)
{
    char path[ASSETS_PATH_MAX];

    if (assets_path(path, sizeof path, relative) != 0) {
        fprintf(stderr, "asset path is too long: %s\n", relative);
        return -1;
    }

    return obj_load(model, path);
}

static int upload_model(World *world, const Model *model, Mesh **mesh, Material **materials)
{
    int i;

    if (world->mesh_count == WORLD_MAX_MESHES ||
        world->material_count + model->material_count > WORLD_MAX_MATERIALS) {
        fprintf(stderr, "the airbase has no room for %s\n", model->directory);
        return -1;
    }
    *mesh = &world->meshes[world->mesh_count];
    *materials = &world->materials[world->material_count];
    if (mesh_create(*mesh, &model->data) != 0) {
        fprintf(stderr, "out of memory uploading %s\n", model->directory);
        return -1;
    }
    world->mesh_count++;
    world->material_count += model->material_count;
    for (i = 0; i < model->material_count; i++) {
        material_load(&(*materials)[i], &model->materials[i], model->directory);
    }

    return 0;
}

// nothing is measured off these models beyond their mesh, so the parsed data goes as soon as it is uploaded
int world_load(World *world, const char *relative, Mesh **mesh, Material **materials)
{
    Model model;
    int uploaded;

    if (open_model(&model, relative) != 0) {
        return -1;
    }
    uploaded = upload_model(world, &model, mesh, materials);
    obj_free(&model);

    return uploaded;
}

static Mesh *build_mesh(World *world, MeshData *data)
{
    Mesh *mesh = NULL;

    if (world->mesh_count < WORLD_MAX_MESHES && mesh_create(&world->meshes[world->mesh_count], data) == 0) {
        mesh = &world->meshes[world->mesh_count++];
    } else {
        fprintf(stderr, "out of memory building the airbase\n");
    }
    mesh_data_free(data);

    return mesh;
}

static Material *plain_material(World *world, Vec3 kd, float roughness, float metallic)
{
    Material *material;

    if (world->material_count == WORLD_MAX_MATERIALS) {
        fprintf(stderr, "the airbase has no room for another material\n");
        return NULL;
    }
    material = &world->materials[world->material_count++];
    material_default(material);
    material->kd = kd;
    material->roughness = roughness;
    material->metallic = metallic;

    return material;
}

static GLuint named_texture(const char *prefix, const char *suffix, TextureKind kind)
{
    char relative[ASSETS_PATH_MAX];
    char path[ASSETS_PATH_MAX];

    snprintf(relative, sizeof relative, "%s%s", prefix, suffix);
    if (assets_path(path, sizeof path, relative) != 0) {
        fprintf(stderr, "asset path is too long: %s\n", relative);
        return 0;
    }

    return material_texture(path, kind);
}

// every collected ground set is named <prefix>_diffuse_2k.jpg and so on
static Material *ground_material(World *world, const char *prefix)
{
    Material *material = plain_material(world, v3(1.0f, 1.0f, 1.0f), 1.0f, 0.0f);

    if (material == NULL) {
        return NULL;
    }
    material->diffuse_map = named_texture(prefix, "_diffuse_2k.jpg", TEXTURE_COLOR);
    material->normal_map = named_texture(prefix, "_normal_gl_1k.jpg", TEXTURE_DATA);
    // a gray roughness image has the roughness in every channel and fits the glTF packing as it is
    material->metal_rough_map = named_texture(prefix, "_roughness_1k.jpg", TEXTURE_DATA);
    if (material->diffuse_map == 0 || material->normal_map == 0 || material->metal_rough_map == 0) {
        return NULL;
    }

    return material;
}

static Mesh *build_paint(World *world)
{
    const Vec3 up = v3(0.0f, 1.0f, 0.0f);
    const Vec3 right = v3(1.0f, 0.0f, 0.0f);
    const float threshold = WORLD_RUNWAY_LENGTH * 0.5f - PIANO_INSET - PIANO_LENGTH * 0.5f;
    MeshData data;
    int slot;
    int stripe;

    if (mesh_data_reserve(&data, PAINT_RECTS * 4, PAINT_RECTS * 6, 1) != 0) {
        fprintf(stderr, "out of memory painting the runway\n");
        return NULL;
    }
    data.groups[0].index_count = PAINT_RECTS * 6;

    for (slot = 0; slot < DASH_COUNT; slot++) {
        const float z = -DASH_SPAN * 0.5f + DASH_PERIOD * (float)slot;

        mesh_data_rect(&data, slot, v3(0.0f, PAINT_LIFT, z), up, right, DASH_WIDTH * 0.5f, DASH_LENGTH * 0.5f);
    }
    for (stripe = 0; stripe < 2 * PIANO_COUNT; stripe++) {
        const float x = ((float)(stripe % PIANO_COUNT) - (float)(PIANO_COUNT - 1) * 0.5f) * PIANO_WIDTH * 2.0f;
        const float z = stripe < PIANO_COUNT ? -threshold : threshold;

        mesh_data_rect(&data, DASH_COUNT + stripe, v3(x, PAINT_LIFT, z), up, right, PIANO_WIDTH * 0.5f,
                       PIANO_LENGTH * 0.5f);
    }

    return build_mesh(world, &data);
}

static Mesh *build_roundels(World *world)
{
    const Vec3 up = v3(0.0f, 1.0f, 0.0f);
    const Vec3 nose = v3(0.0f, 0.0f, 1.0f);
    MeshData data;

    if (mesh_data_reserve(&data, ROUNDEL_COUNT * 4, ROUNDEL_COUNT * 6, 1) != 0) {
        fprintf(stderr, "out of memory drawing the markings\n");
        return NULL;
    }
    data.groups[0].index_count = ROUNDEL_COUNT * 6;

    mesh_data_rect(&data, 0, v3(ROUNDEL_WING_X, ROUNDEL_WING_Y, ROUNDEL_WING_Z), up, v3(1.0f, 0.0f, 0.0f),
                   ROUNDEL_WING_HALF, ROUNDEL_WING_HALF);
    mesh_data_rect(&data, 1, v3(-ROUNDEL_WING_X, ROUNDEL_WING_Y, ROUNDEL_WING_Z), up, v3(1.0f, 0.0f, 0.0f),
                   ROUNDEL_WING_HALF, ROUNDEL_WING_HALF);
    mesh_data_rect(&data, 2, v3(ROUNDEL_SIDE_X, ROUNDEL_SIDE_Y, ROUNDEL_SIDE_Z), v3(1.0f, 0.0f, 0.0f), nose,
                   ROUNDEL_SIDE_HALF, ROUNDEL_SIDE_HALF);
    mesh_data_rect(&data, 3, v3(-ROUNDEL_SIDE_X, ROUNDEL_SIDE_Y, ROUNDEL_SIDE_Z), v3(-1.0f, 0.0f, 0.0f), nose,
                   ROUNDEL_SIDE_HALF, ROUNDEL_SIDE_HALF);

    return build_mesh(world, &data);
}

static Mesh *build_gear(World *world)
{
    const float main_axle = MAIN_WHEEL_RADIUS - WORLD_GEAR_DROP;
    const float nose_axle = NOSE_WHEEL_RADIUS - WORLD_GEAR_DROP;
    MeshData data;
    int leg;

    if (mesh_data_reserve(&data, GEAR_TUBES * MESH_DATA_TUBE_VERTICES, GEAR_TUBES * MESH_DATA_TUBE_INDICES, 2) != 0) {
        fprintf(stderr, "out of memory building the landing gear\n");
        return NULL;
    }
    data.groups[0].index_count = GEAR_LEGS * MESH_DATA_TUBE_INDICES;
    data.groups[1].material = 1;
    data.groups[1].first_index = GEAR_LEGS * MESH_DATA_TUBE_INDICES;
    data.groups[1].index_count = GEAR_LEGS * MESH_DATA_TUBE_INDICES;

    for (leg = 0; leg < 2; leg++) {
        // the legs splay out from the wheel wells, so the wheels stand wider than the belly is broad
        const float side = leg == 0 ? 1.0f : -1.0f;
        const Vec3 top = v3(side * MAIN_LEG_X, MAIN_LEG_TOP, MAIN_LEG_Z);
        const Vec3 axle = v3(side * MAIN_TRACK, main_axle, MAIN_LEG_Z);

        mesh_data_tube(&data, leg, top, axle, STRUT_RADIUS);
        mesh_data_tube(&data, GEAR_LEGS + leg, v3_add(axle, v3(-MAIN_WHEEL_HALF_WIDTH, 0.0f, 0.0f)),
                       v3_add(axle, v3(MAIN_WHEEL_HALF_WIDTH, 0.0f, 0.0f)), MAIN_WHEEL_RADIUS);
    }
    mesh_data_tube(&data, GEAR_LEGS - 1, v3(0.0f, NOSE_LEG_TOP, NOSE_LEG_Z), v3(0.0f, nose_axle, NOSE_LEG_Z),
                   NOSE_STRUT_RADIUS);
    mesh_data_tube(&data, GEAR_TUBES - 1, v3(-NOSE_WHEEL_HALF_WIDTH, nose_axle, NOSE_LEG_Z),
                   v3(NOSE_WHEEL_HALF_WIDTH, nose_axle, NOSE_LEG_Z), NOSE_WHEEL_RADIUS);

    return build_mesh(world, &data);
}

static Mesh *paved_mesh(World *world, float size_x, float size_z, float tile)
{
    MeshData data;

    if (mesh_data_quad(&data, size_x, size_z, tile) != 0) {
        fprintf(stderr, "out of memory paving the airbase\n");
        return NULL;
    }

    return build_mesh(world, &data);
}

static Mesh *build_cone(World *world)
{
    MeshData data;

    if (mesh_data_cone(&data, CONE_RADIUS, CONE_HEIGHT, CONE_SIDES) != 0) {
        fprintf(stderr, "out of memory building the marker cones\n");
        return NULL;
    }

    return build_mesh(world, &data);
}

static Entity *place(Scene *scene, const Mesh *mesh, const Material *materials, Vec3 position, float yaw,
                     float scale)
{
    Entity *entity = scene_add(scene, mesh, materials);

    if (entity == NULL) {
        fprintf(stderr, "the scene has no room for another object\n");
        return NULL;
    }
    entity->position = position;
    entity->orientation = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), yaw);
    entity->scale = scale;

    return entity;
}

// flat paving shades itself badly in a depth pass that keeps back faces only, and nothing lies under it
static int pave(Scene *scene, const Mesh *mesh, const Material *material, Vec3 position)
{
    Entity *entity = place(scene, mesh, material, position, 0.0f, 1.0f);

    if (entity == NULL) {
        return -1;
    }
    entity->casts_shadow = 0;

    return 0;
}

static Prop *add_prop(World *world, Entity *entity, float yaw, float rest, PropKind kind)
{
    Prop *prop;

    if (entity == NULL) {
        return NULL;
    }
    if (world->prop_count == WORLD_MAX_PROPS) {
        fprintf(stderr, "the apron has no room for another object\n");
        return NULL;
    }
    prop = &world->props[world->prop_count++];
    prop->entity = entity;
    prop->min = v3_scale(entity->mesh->bounds_min, entity->scale);
    prop->max = v3_scale(entity->mesh->bounds_max, entity->scale);
    prop->yaw = yaw;
    prop->rest = rest;
    prop->kind = kind;

    return prop;
}

static int load_stores(World *world)
{
    Mesh *mesh;
    Material *materials;

    if (world_load(world, JSOW_MODEL, &mesh, &materials) != 0) {
        return -1;
    }
    // the metal-rough maps read almost fully metallic and mirror too much sky; every model gets this scale
    materials[0].metallic *= 0.4f;
    world->missile_mesh = mesh;
    world->missile_materials = materials;

    return 0;
}

// the cones stand around the jet the way the ground crew would put them, as offsets from the parking spot
static const float cone_spots[CONE_COUNT][2] = {{-2.0f, 9.5f}, {4.5f, 7.5f}, {-7.5f, 11.0f}};

static int load_ground(World *world, Scene *scene)
{
    Material *apron_material = ground_material(world, APRON_TEXTURE);
    Material *runway_material = ground_material(world, RUNWAY_TEXTURE);
    Material *paint_material = plain_material(world, v3(0.82f, 0.82f, 0.79f), 0.75f, 0.0f);
    Material *cone_material = plain_material(world, v3(0.95f, 0.28f, 0.04f), 0.55f, 0.0f);
    Mesh *apron_mesh = paved_mesh(world, APRON_SIZE, APRON_SIZE, APRON_TILE);
    Mesh *runway_mesh = paved_mesh(world, WORLD_RUNWAY_WIDTH, WORLD_RUNWAY_LENGTH, RUNWAY_TILE);
    Mesh *taxiway_mesh = paved_mesh(world, APRON_EDGE - RUNWAY_EDGE, TAXIWAY_WIDTH, RUNWAY_TILE);
    Mesh *paint_mesh = build_paint(world);
    Mesh *cone_mesh = build_cone(world);
    int i;

    if (apron_material == NULL || runway_material == NULL || paint_material == NULL || cone_material == NULL ||
        apron_mesh == NULL || runway_mesh == NULL || taxiway_mesh == NULL || paint_mesh == NULL ||
        cone_mesh == NULL) {
        return -1;
    }

    if (pave(scene, apron_mesh, apron_material, v3(WORLD_APRON_X, WORLD_PAVING, APRON_Z)) != 0 ||
        pave(scene, runway_mesh, runway_material, v3(0.0f, WORLD_PAVING, 0.0f)) != 0 ||
        pave(scene, taxiway_mesh, runway_material,
             v3((APRON_EDGE + RUNWAY_EDGE) * 0.5f, WORLD_PAVING, WORLD_TAXI_Z)) != 0 ||
        pave(scene, paint_mesh, paint_material, v3(0.0f, WORLD_PAVING, 0.0f)) != 0) {
        return -1;
    }

    for (i = 0; i < CONE_COUNT; i++) {
        const Vec3 spot = v3(WORLD_APRON_X + cone_spots[i][0], WORLD_PAVING, WORLD_PARK_Z + cone_spots[i][1]);
        Entity *cone = place(scene, cone_mesh, cone_material, spot, 0.0f, 1.0f);

        if (add_prop(world, cone, 0.0f, WORLD_PAVING, PROP_DRAGGABLE) == NULL) {
            return -1;
        }
    }

    return 0;
}

static int load_shelters(World *world, Scene *scene)
{
    const float shelter_y = WORLD_PAVING - SHELTER_SINK;
    Mesh *mesh;
    Material *materials;
    int i;

    if (world_load(world, SHELTER_MODEL, &mesh, &materials) != 0) {
        return -1;
    }
    for (i = 0; i < 2; i++) {
        const Vec3 spot = v3(WORLD_APRON_X + SHELTER_GAP * (float)i, shelter_y, WORLD_PARK_Z - SHELTER_BACK);
        Entity *shelter = place(scene, mesh, materials, spot, 0.0f, SHELTER_SCALE);

        if (add_prop(world, shelter, 0.0f, shelter_y, PROP_FIXED) == NULL) {
            return -1;
        }
    }

    return 0;
}

static int load_cart(World *world, Scene *scene)
{
    const float missile_half = world->missile_mesh->bounds_max.y * JSOW_SCALE;
    Model model;
    Mesh *mesh;
    Material *materials;
    Entity *cart;
    Prop *prop;
    Vec3 cradle_min;
    Vec3 cradle_max;
    float load_y;
    int load_group;
    int frame_group;
    int i;

    if (open_model(&model, CART_MODEL) != 0) {
        return -1;
    }
    if (upload_model(world, &model, &mesh, &materials) != 0) {
        obj_free(&model);
        return -1;
    }
    cart = place(scene, mesh, materials, v3(WORLD_APRON_X + CART_OFFSET_X, WORLD_PAVING, WORLD_PARK_Z + CART_OFFSET_Z),
                 0.0f, 1.0f);
    load_group = mesh_group_index(mesh, CART_LOAD_GROUP);
    frame_group = mesh_group_index(mesh, CART_FRAME_GROUP);
    prop = add_prop(world, cart, 0.0f, WORLD_PAVING, PROP_CARRIER);
    if (prop == NULL) {
        obj_free(&model);
        return -1;
    }
    if (load_group < 0 || frame_group < 0) {
        fprintf(stderr, "%s is not the two group model the cradle was measured from\n", CART_MODEL);
        obj_free(&model);
        return -1;
    }
    cart->group_flags[load_group] = GROUP_HIDDEN;
    mesh_data_group_bounds(&model.data, load_group, &cradle_min, &cradle_max);
    // the mesh box counts the R-27s the cart was drawn with, and those are hidden, so the frame gives the box
    mesh_data_group_bounds(&model.data, frame_group, &prop->min, &prop->max);
    prop->min = v3_scale(prop->min, cart->scale);
    prop->max = v3_scale(prop->max, cart->scale);
    obj_free(&model);

    load_y = cart->position.y + cradle_min.y * cart->scale + missile_half;
    for (i = 0; i < WORLD_PYLONS; i++) {
        const float offset = ((float)i - (float)(WORLD_PYLONS - 1) * 0.5f) * CART_LOAD_SPACING;
        const Vec3 spot = v3(cart->position.x + CART_LOAD_X, load_y, cart->position.z + offset);
        // the missiles lie along the cart, so their noses point across the jet's own heading
        Entity *missile =
            place(scene, world->missile_mesh, world->missile_materials, spot, VEC_PI * 0.5f, JSOW_SCALE);
        Prop *missile_prop = add_prop(world, missile, VEC_PI * 0.5f, WORLD_PAVING + missile_half, PROP_MISSILE);

        if (missile_prop == NULL) {
            return -1;
        }
        // a rearm sends a fired missile back to the slot it started in, wherever the cart has moved since
        missile_prop->home = spot;
    }

    return 0;
}

static int load_ladder(World *world, Scene *scene)
{
    const Vec3 spot = v3(WORLD_APRON_X + LADDER_OFFSET_X, WORLD_PAVING, WORLD_PARK_Z + LADDER_OFFSET_Z);
    Mesh *mesh;
    Material *materials;
    Entity *ladder;

    if (world_load(world, LADDER_MODEL, &mesh, &materials) != 0) {
        return -1;
    }
    ladder = place(scene, mesh, materials, spot, 0.0f, LADDER_SCALE);
    if (add_prop(world, ladder, 0.0f, WORLD_PAVING, PROP_DRAGGABLE) == NULL) {
        return -1;
    }
    ladder->group_flags[0] = GROUP_DOUBLE_SIDED;

    return 0;
}

// the canopy is the only glass on the model and the only part that moves, so it is flagged and measured at once
static void measure_canopy(World *world, const Model *model)
{
    // the box starts inverted, which leaves the eye in the middle of a jet whose canopy groups are missing
    Vec3 min = world->jet->mesh->bounds_max;
    Vec3 max = world->jet->mesh->bounds_min;
    int i;

    for (i = 0; i < 2; i++) {
        const int group = mesh_group_index(world->jet->mesh, canopy_groups[i]);
        Vec3 group_min;
        Vec3 group_max;

        if (group < 0) {
            fprintf(stderr, "%s has no group %s\n", JET_MODEL, canopy_groups[i]);
            continue;
        }
        world->jet->group_flags[group] = GROUP_BLENDED | GROUP_DOUBLE_SIDED | GROUP_HINGED;
        mesh_data_group_bounds(&model->data, group, &group_min, &group_max);
        min = v3(fminf(min.x, group_min.x), fminf(min.y, group_min.y), fminf(min.z, group_min.z));
        max = v3(fmaxf(max.x, group_max.x), fmaxf(max.y, group_max.y), fmaxf(max.z, group_max.z));
    }
    world->cockpit_eye = v3_scale(v3_lerp(min, max, 0.5f), JET_SCALE);
    // the canopy swings up about its rear sill
    world->canopy_pivot = v3(0.0f, min.y, min.z);
}

// a missile hangs centered under its holder with its back flush against the holder's underside
static Vec3 holder_offset(const Model *model, const Mesh *mesh, const char *name, float missile_half)
{
    const int group = mesh_group_index(mesh, name);
    Vec3 min;
    Vec3 max;

    if (group < 0) {
        fprintf(stderr, "%s has no group %s\n", JET_MODEL, name);
        return v3(0.0f, 0.0f, 0.0f);
    }
    mesh_data_group_bounds(&model->data, group, &min, &max);

    return v3((min.x + max.x) * 0.5f * JET_SCALE, min.y * JET_SCALE - missile_half,
              (min.z + max.z) * 0.5f * JET_SCALE);
}

// center of the nozzle group's aft face
static Vec3 measure_nozzle(const Model *model, const Mesh *mesh)
{
    const int group = mesh_group_index(mesh, "nozzle");
    Vec3 min;
    Vec3 max;

    if (group < 0) {
        fprintf(stderr, "%s has no group nozzle\n", JET_MODEL);
        return v3(0.0f, 0.0f, 0.0f);
    }
    mesh_data_group_bounds(&model->data, group, &min, &max);

    return v3((min.x + max.x) * 0.5f * JET_SCALE, (min.y + max.y) * 0.5f * JET_SCALE, min.z * JET_SCALE);
}

static int load_rails(World *world, Scene *scene, const Model *jet_model)
{
    Mesh *mesh;
    Material *materials;
    int i;

    if (world_load(world, AIM9_MODEL, &mesh, &materials) != 0) {
        return -1;
    }
    // same metalness scale as the store missiles
    materials[0].metallic *= 0.4f;

    for (i = 0; i < 2; i++) {
        world->rail_offsets[i] = holder_offset(jet_model, world->jet->mesh, rail_groups[i],
                                               mesh->bounds_max.y * AIM9_SCALE);
        world->rails[i] = place(scene, mesh, materials, v3(0.0f, 0.0f, 0.0f), 0.0f, AIM9_SCALE);
        if (world->rails[i] == NULL) {
            return -1;
        }
    }

    return 0;
}

static int load_jet(World *world, Scene *scene)
{
    const float missile_half = world->missile_mesh->bounds_max.y * JSOW_SCALE;
    // the two are taken in order, so the gear mesh's second group finds the rubber right after the metal
    Material *legs = plain_material(world, v3(0.60f, 0.62f, 0.64f), 0.35f, 0.8f);
    Material *tires = plain_material(world, v3(0.02f, 0.02f, 0.022f), 1.0f, 0.0f);
    Material *paint = plain_material(world, v3(1.0f, 1.0f, 1.0f), 0.6f, 0.0f);
    Model model;
    Mesh *mesh;
    Material *materials;
    Mesh *gear_mesh;
    Mesh *roundel_mesh;
    Prop *prop;
    int i;

    if (legs == NULL || tires == NULL || paint == NULL) {
        return -1;
    }
    paint->diffuse_map = named_texture(ROUNDEL_TEXTURE, "_1024.png", TEXTURE_CUTOUT);
    gear_mesh = build_gear(world);
    roundel_mesh = build_roundels(world);
    if (paint->diffuse_map == 0 || gear_mesh == NULL || roundel_mesh == NULL) {
        return -1;
    }

    if (open_model(&model, JET_MODEL) != 0) {
        return -1;
    }
    if (upload_model(world, &model, &mesh, &materials) != 0) {
        obj_free(&model);
        return -1;
    }
    // same metalness scale as the store missiles
    materials[0].metallic *= 0.4f;
    // world_park puts it on its spot once everything that rides on it exists
    world->jet = place(scene, mesh, materials, v3(0.0f, 0.0f, 0.0f), WORLD_PARK_HEADING, JET_SCALE);
    if (world->jet == NULL) {
        obj_free(&model);
        return -1;
    }
    measure_canopy(world, &model);
    world->nozzle_offset = measure_nozzle(&model, mesh);
    for (i = 0; i < WORLD_PYLONS; i++) {
        world->pylons[i] = holder_offset(&model, mesh, pylon_groups[i], missile_half);
    }

    world->gear = place(scene, gear_mesh, legs, v3(0.0f, 0.0f, 0.0f), 0.0f, 1.0f);
    world->roundels = place(scene, roundel_mesh, paint, v3(0.0f, 0.0f, 0.0f), 0.0f, 1.0f);
    if (world->gear == NULL || world->roundels == NULL || load_rails(world, scene, &model) != 0) {
        obj_free(&model);
        return -1;
    }
    obj_free(&model);
    world->roundels->group_flags[0] = GROUP_BLENDED;
    world->roundels->casts_shadow = 0;

    world->jet_min = v3_scale(mesh->bounds_min, JET_SCALE);
    world->jet_max = v3_scale(mesh->bounds_max, JET_SCALE);
    world->jet_center = v3_lerp(world->jet_min, world->jet_max, 0.5f);
    prop = add_prop(world, world->jet, WORLD_PARK_HEADING, WORLD_STAND, PROP_CARRIER);
    if (prop == NULL) {
        return -1;
    }
    // the legs hang below the model's own box, and taking hold of a wheel should take hold of the jet
    prop->min.y -= WORLD_GEAR_DROP;

    return 0;
}

int world_init(World *world, Scene *scene, Aircraft *aircraft)
{
    if (load_stores(world) != 0 || load_ground(world, scene) != 0 || load_shelters(world, scene) != 0 ||
        load_cart(world, scene) != 0 || load_ladder(world, scene) != 0 || load_jet(world, scene) != 0) {
        return -1;
    }
    world_park(world, aircraft);

    return 0;
}

void world_park(World *world, Aircraft *aircraft)
{
    aircraft_place(aircraft, v3(WORLD_APRON_X, WORLD_STAND, WORLD_PARK_Z), WORLD_PARK_HEADING, 0.0f, 0.0f);
    world->jet->position = aircraft->position;
    world->jet->orientation = aircraft->orientation;
    world_follow(world);
}

void world_canopy(World *world, float shut)
{
    const Vec3 pivot = world->canopy_pivot;
    const Mat4 swing = m4_rotate(-CANOPY_OPEN * (1.0f - shut), v3(1.0f, 0.0f, 0.0f));

    world->jet->hinge = m4_multiply(m4_translate(pivot),
                                    m4_multiply(swing, m4_translate(v3_scale(pivot, -1.0f))));
}

Vec3 world_on_jet(const World *world, Vec3 offset)
{
    return v3_add(world->jet->position, quat_rotate(world->jet->orientation, offset));
}

static void ride_jet(World *world, Entity *entity, Vec3 offset)
{
    entity->position = world_on_jet(world, offset);
    entity->orientation = world->jet->orientation;
    entity->highlight_color = world->jet->highlight_color;
    entity->highlight = world->jet->highlight;
}

void world_follow(World *world)
{
    const Vec3 origin = v3(0.0f, 0.0f, 0.0f);
    int i;

    ride_jet(world, world->gear, origin);
    ride_jet(world, world->roundels, origin);
    for (i = 0; i < 2; i++) {
        ride_jet(world, world->rails[i], world->rail_offsets[i]);
    }
}
