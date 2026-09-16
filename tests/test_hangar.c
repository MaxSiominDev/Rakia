#include "check.h"
#include "engine/vecmath.h"
#include "game/hangar.h"

#include <string.h>

#define JET_HALF v3(4.7f, 2.3f, 7.5f)
#define MISSILE_HALF v3(0.28f, 0.22f, 2.05f)
#define CONE_HALF v3(0.28f, 0.38f, 0.28f)
#define CART_HALF v3(2.5f, 0.55f, 1.0f)
#define MISSILE_REST 0.37f

static Scene scene;
static World world;
static Hangar hangar;
static Mesh shell;
static MeshGroup group;

// a prop is an entity with a box around it, so the tests place plain entities and measure them by hand
static Prop *add(Vec3 position, Vec3 half, float rest, PropKind kind)
{
    Prop *prop = &world.props[world.prop_count++];

    prop->entity = scene_add(&scene, &shell, NULL);
    prop->entity->position = position;
    prop->min = v3_scale(half, -1.0f);
    prop->max = half;
    prop->yaw = 0.0f;
    prop->rest = rest;
    prop->kind = kind;

    return prop;
}

// straight down onto a point, so the ray meets any horizontal plane right above or below it
static Ray onto(float x, float z)
{
    Ray ray;

    ray.origin = v3(x, 60.0f, z);
    ray.direction = v3(0.0f, -1.0f, 0.0f);

    return ray;
}

// the apron with the jet at the origin, a missile in a cart beside it, another one further out and a cone
static void apron(void)
{
    memset(&scene, 0, sizeof scene);
    memset(&world, 0, sizeof world);
    memset(&shell, 0, sizeof shell);
    memset(&group, 0, sizeof group);
    group.index_count = 3;
    shell.groups = &group;
    shell.group_count = 1;

    world.jet = add(v3(0.0f, 0.95f, 0.0f), JET_HALF, 0.95f, PROP_CARRIER)->entity;
    add(v3(12.0f, 0.92f, 0.0f), MISSILE_HALF, MISSILE_REST, PROP_MISSILE);
    add(v3(12.0f, 0.92f, 6.0f), MISSILE_HALF, MISSILE_REST, PROP_MISSILE);
    add(v3(20.0f, 0.38f, 0.0f), CONE_HALF, 0.38f, PROP_DRAGGABLE);
    add(v3(0.0f, 0.0f, -34.0f), v3(8.0f, 9.0f, 12.5f), 0.0f, PROP_FIXED);
    // the cart stands taller than the missiles it holds, so its box is what a ray from above meets first
    add(v3(12.0f, 0.70f, 0.0f), CART_HALF, 0.15f, PROP_CARRIER);
    // spread wider apart than the real jet's, so each of these tests names one pylon and no other
    world.pylons[0] = v3(3.0f, 0.7f, -2.0f);
    world.pylons[1] = v3(-3.0f, 0.7f, -2.0f);
    world.pylons[2] = v3(3.0f, 0.7f, 3.0f);
    world.pylons[3] = v3(-3.0f, 0.7f, 3.0f);
    hangar_init(&hangar);
}

static float tint_of(int prop)
{
    return world.props[prop].entity->highlight;
}

static void test_hover_and_grab(void)
{
    float hover;

    apron();
    hangar_step(&hangar, &world, onto(40.0f, 40.0f), 0);
    check(hangar.hovered == -1 && tint_of(1) == 0.0f, "empty apron under the cursor tints nothing");

    hangar_step(&hangar, &world, onto(12.0f, 0.0f), 0);
    hover = tint_of(1);
    check(hangar.hovered == 1 && hover > 0.0f, "hovering a missile tints it");
    check(hangar.dragged == -1, "without taking hold of it");

    hangar_step(&hangar, &world, onto(12.0f, 0.0f), 1);
    check(hangar.dragged == 1, "pressing the button takes hold");
    check(tint_of(1) > hover, "and tints it harder");

    hangar_step(&hangar, &world, onto(12.0f, 0.0f), 0);
    check(hangar.dragged == -1, "letting go drops it");
    check_close(tint_of(1), hover, 1e-6f, "and gives the color back");

    hangar_step(&hangar, &world, onto(40.0f, 40.0f), 0);
    check(tint_of(1) == 0.0f, "moving the cursor away clears the tint for good");

    hangar_step(&hangar, &world, onto(0.0f, -34.0f), 0);
    check(hangar.hovered == -1, "a shelter is scenery and does not answer the cursor");
}

static void test_drag(void)
{
    apron();
    // the cursor takes hold off the missile's own origin, near its nose
    hangar_step(&hangar, &world, onto(12.2f, 1.0f), 0);
    hangar_step(&hangar, &world, onto(12.2f, 1.0f), 1);
    check(hangar.dragged == 1, "the missile is held");
    check_v3(world.props[1].entity->position, 12.0f, 0.92f, 0.0f, "and has not moved yet");

    hangar_step(&hangar, &world, onto(6.2f, 10.0f), 1);
    check_v3(world.props[1].entity->position, 6.0f, 0.92f, 9.0f,
             "dragging moves it with the cursor, keeping the grip it was taken by");

    hangar_step(&hangar, &world, onto(6.2f, 10.0f), 0);
    check_close(world.props[1].entity->position.y, MISSILE_REST, 1e-6f, "and dropping it settles it on the apron");
    check(hangar_missiles(&hangar) == 0, "with nothing hanging on the jet");
}

static void drag_to(int prop, float from_x, float from_z, float to_x, float to_z)
{
    hangar_step(&hangar, &world, onto(from_x, from_z), 0);
    check(hangar.hovered == prop, "the cursor finds the prop it was aimed at");
    hangar_step(&hangar, &world, onto(from_x, from_z), 1);
    hangar_step(&hangar, &world, onto(to_x, to_z), 1);
    hangar_step(&hangar, &world, onto(to_x, to_z), 0);
}

static void test_pylon_snap(void)
{
    apron();
    drag_to(1, 12.0f, 0.0f, 3.0f, -2.0f);
    check(hangar.mounted[0] == 1 && hangar_missiles(&hangar) == 1, "a missile let go under a pylon hangs itself on it");
    check_v3(world.props[1].entity->position, 3.0f, 1.65f, -2.0f,
             "right where the pylon holds it, which is a pylon leg above the jet's own origin");

    drag_to(2, 12.0f, 6.0f, 14.0f, 10.0f);
    check(hangar_missiles(&hangar) == 1, "let go out in the open it stays on the apron");
    check_close(world.props[2].entity->position.y, MISSILE_REST, 1e-6f, "lying where it was dropped");
    check_close(world.props[2].entity->position.x, 14.0f, 1e-5f, "and not anywhere else");

    drag_to(2, 14.0f, 10.0f, -3.0f, -2.0f);
    check(hangar.mounted[1] == 2 && hangar_missiles(&hangar) == 2, "the pylon on the other wing takes the second one");

    drag_to(1, 3.0f, -2.0f, 14.0f, 0.0f);
    check(hangar.mounted[0] == -1 && hangar_missiles(&hangar) == 1, "dragging a missile off a pylon frees it again");
    check_close(world.props[1].entity->position.y, MISSILE_REST, 1e-6f, "and puts it back on the apron");
}

static void test_carry(void)
{
    apron();
    drag_to(1, 12.0f, 0.0f, 3.0f, -2.0f);

    world.jet->position = v3(100.0f, 0.95f, 250.0f);
    // a quarter turn about +y takes the nose toward +x, so the pylon that was 2 m astern swings 2 m east
    world.jet->orientation = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), VEC_PI * 0.5f);
    hangar_carry(&hangar, &world);
    check_v3(world.props[1].entity->position, 98.0f, 1.65f, 247.0f,
             "the mounted missile goes wherever the jet goes, turned with it");
}

static void test_push_apart(void)
{
    apron();
    hangar_step(&hangar, &world, onto(20.0f, 0.0f), 0);
    hangar_step(&hangar, &world, onto(20.0f, 0.0f), 1);
    hangar_step(&hangar, &world, onto(12.0f, 6.0f), 1);
    check(world.props[3].entity->position.x > 12.3f,
          "a cone dragged onto a missile is pushed back out along the shorter way");
    check_close(world.props[3].entity->position.z, 6.0f, 1e-5f, "and not along the missile's own length");

    hangar_step(&hangar, &world, onto(0.0f, 0.0f), 1);
    check(world.props[3].entity->position.x > 4.9f, "and a cone dragged into the jet stops at its wing");

    apron();
    // the cart parked just far enough past the missile to leave the cone a slot between them
    world.props[5].entity->position = v3(12.0f, 0.70f, 3.7f);
    hangar_step(&hangar, &world, onto(20.0f, 0.0f), 0);
    hangar_step(&hangar, &world, onto(20.0f, 0.0f), 1);
    hangar_step(&hangar, &world, onto(12.0f, 2.0f), 1);
    check_close(world.props[3].entity->position.z, MISSILE_HALF.z + CONE_HALF.z, 1e-5f,
                "a cone dragged at the missile ends up in the slot, clear of the cart behind it too");
}

static void test_high_pylon(void)
{
    apron();
    // the missile is dragged at the height it lay on the cart, a good half metre under the pylon it reaches
    world.pylons[0] = v3(3.0f, 2.4f, -2.0f);
    drag_to(1, 12.0f, 0.0f, 3.0f, -2.0f);
    check(hangar.mounted[0] == 1, "a missile under a high pylon still finds it, since the drag height is its own");
}

static void test_cart(void)
{
    apron();
    hangar_step(&hangar, &world, onto(12.0f, 0.0f), 0);
    check(hangar.hovered == 1, "the cursor finds the missile in the cradle, not the cart around it");

    hangar_step(&hangar, &world, onto(12.0f, 0.0f), 1);
    hangar_step(&hangar, &world, onto(12.0f, 0.3f), 1);
    check_close(world.props[1].entity->position.x, 12.0f, 1e-5f, "and lifting it does not shove it off the cart");
    check_close(world.props[1].entity->position.z, 0.3f, 1e-5f, "it just follows the cursor");

    // the first of these drops the missile, the second is the cursor arriving over the cone
    hangar_step(&hangar, &world, onto(20.0f, 0.0f), 0);
    hangar_step(&hangar, &world, onto(20.0f, 0.0f), 0);
    check(hangar.hovered == 3, "the cone beside the cart answers the cursor as itself");
}

static void test_pass_through(void)
{
    apron();
    hangar_step(&hangar, &world, onto(12.0f, 0.0f), 0);
    hangar_step(&hangar, &world, onto(12.0f, 0.0f), 1);
    hangar_step(&hangar, &world, onto(3.0f, -2.0f), 1);
    check_close(world.props[1].entity->position.x, 3.0f, 1e-5f,
                "a missile passes through the jet, or it could never reach a pylon");

    hangar_step(&hangar, &world, onto(3.0f, -2.0f), 0);
    hangar_step(&hangar, &world, onto(12.0f, 6.0f), 0);
    hangar_step(&hangar, &world, onto(12.0f, 6.0f), 1);
    hangar_step(&hangar, &world, onto(3.5f, -2.0f), 1);
    check_close(world.props[2].entity->position.x, 3.5f, 1e-5f, "and through the one already hanging there");
}

static void test_takeoff_snapshot(void)
{
    apron();
    drag_to(1, 12.0f, 0.0f, 3.0f, -2.0f);
    hangar_takeoff(&hangar);
    check(hangar.takeoff[0] == 1, "the snapshot remembers which prop rode which pylon");
    check(hangar.takeoff[1] == -1, "and which pylons were empty");
}

static void test_restock_pylons(void)
{
    apron();
    drag_to(1, 12.0f, 0.0f, 3.0f, -2.0f);
    hangar_takeoff(&hangar);

    // the missile fires: the pylon empties and the entity is hidden wherever it resolved
    hangar.mounted[0] = -1;
    world.props[1].entity->hidden = 1;
    world.props[1].entity->position = v3(500.0f, 0.0f, 900.0f);

    hangar_restock_pylons(&hangar, &world);
    check(hangar.mounted[0] == 1, "a missile fired at takeoff returns to the pylon it flew from");
    check(!world.props[1].entity->hidden, "and comes back into view");
}

static void test_restock_cart(void)
{
    apron();
    drag_to(1, 12.0f, 0.0f, 3.0f, -2.0f);
    drag_to(2, 12.0f, 6.0f, 20.0f, 10.0f);
    hangar_takeoff(&hangar);
    check(hangar.takeoff[0] == 1, "the mounted missile is remembered");

    hangar.mounted[0] = -1;
    world.props[1].entity->hidden = 1;
    world.props[1].entity->position = v3(500.0f, 0.0f, 900.0f);
    world.props[1].home = v3(12.0f, 0.92f, 0.0f);

    hangar_restock_cart(&hangar, &world);
    check(hangar.mounted[0] == -1, "a restocked missile does not remount itself");
    check(!world.props[1].entity->hidden, "it comes back into view");
    check_v3(world.props[1].entity->position, 12.0f, 0.92f, 0.0f, "right back in its own cart slot");
    check_v3(world.props[2].entity->position, 20.0f, MISSILE_REST, 10.0f,
             "a missile never mounted this sortie is left where the player put it");
}

static void test_restock_skips_mounted(void)
{
    apron();
    drag_to(1, 12.0f, 0.0f, 3.0f, -2.0f);
    hangar_takeoff(&hangar);

    hangar_restock_cart(&hangar, &world);
    check(hangar.mounted[0] == 1, "a missile still on its pylon is untouched by a cart restock");

    hangar_restock_pylons(&hangar, &world);
    check(hangar.mounted[0] == 1, "or by a pylon restock");
}

void test_hangar_main(void)
{
    test_hover_and_grab();
    test_drag();
    test_pylon_snap();
    test_carry();
    test_push_apart();
    test_high_pylon();
    test_cart();
    test_pass_through();
    test_takeoff_snapshot();
    test_restock_pylons();
    test_restock_cart();
    test_restock_skips_mounted();
}
