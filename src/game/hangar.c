#include "game/hangar.h"

#include "engine/collision.h"

#define HOVER_TINT 0.28f
#define GRAB_TINT 0.65f
// how close a released missile has to come to a free pylon to hang itself on it
#define SNAP_REACH 1.5f

static const Vec3 tint_color = {1.0f, 0.72f, 0.20f};

// the prop's box is already in metres, so the placement alone takes the ray into the box's frame
static Mat4 prop_matrix(const Prop *prop)
{
    return m4_multiply(m4_translate(prop->entity->position), quat_to_mat4(prop->entity->orientation));
}

static Footprint prop_footprint(const Prop *prop)
{
    return collision_footprint(prop->entity->position, prop->yaw, prop->min, prop->max);
}

static int pylon_of(const Hangar *hangar, int prop)
{
    int i;

    for (i = 0; i < WORLD_PYLONS; i++) {
        if (hangar->mounted[i] == prop) {
            return i;
        }
    }

    return -1;
}

// which prop the cursor means when several boxes lie under it: a missile in the cart's cradle or under a wing
// stands inside the box of whatever carries it, so the carrier gives way to everything else
static int pick_rank(const Prop *prop)
{
    if (prop->kind == PROP_MISSILE) {
        return 0;
    }

    return prop->kind == PROP_CARRIER ? 2 : 1;
}

static int pick(const World *world, Ray ray)
{
    float nearest = 0.0f;
    int best = 0;
    int found = -1;
    int i;

    for (i = 0; i < world->prop_count; i++) {
        const Prop *prop = &world->props[i];
        const int rank = pick_rank(prop);
        float distance;

        if (prop->kind == PROP_FIXED ||
            !picking_box(ray, prop_matrix(prop), prop->min, prop->max, &distance)) {
            continue;
        }
        if (found < 0 || rank < best || (rank == best && distance < nearest)) {
            best = rank;
            nearest = distance;
            found = i;
        }
    }

    return found;
}

// a missile lies on the cart and has to reach the pylons under the wing, so it passes through both carriers
// and through whatever already hangs there
static int passes_through(const Hangar *hangar, const World *world, int moving, int other)
{
    const PropKind a = world->props[moving].kind;
    const PropKind b = world->props[other].kind;

    return pylon_of(hangar, other) >= 0 || (a == PROP_MISSILE && b == PROP_CARRIER) ||
           (b == PROP_MISSILE && a == PROP_CARRIER);
}

// each push moves the prop on, so the props after it in the list are met where it has ended up
static void push_clear(const Hangar *hangar, World *world, int index)
{
    Prop *prop = &world->props[index];
    Footprint moving = prop_footprint(prop);
    int i;

    for (i = 0; i < world->prop_count; i++) {
        Footprint standing;
        Vec3 push;

        if (i == index || passes_through(hangar, world, index, i)) {
            continue;
        }
        standing = prop_footprint(&world->props[i]);
        if (collision_push(&moving, &standing, &push)) {
            prop->entity->position = v3_add(prop->entity->position, push);
            moving.center = v3_add(moving.center, push);
        }
    }
}

static void take_hold(Hangar *hangar, World *world, Ray ray, int index)
{
    Prop *prop = &world->props[index];
    const float plane = prop->entity->position.y;
    const int pylon = pylon_of(hangar, index);
    Vec3 hit;

    if (!picking_ground(ray, plane, &hit)) {
        return;
    }
    if (pylon >= 0) {
        hangar->mounted[pylon] = -1;
    }
    // a missile taken off a rail comes away level again
    prop->entity->orientation = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), prop->yaw);
    hangar->dragged = index;
    hangar->plane = plane;
    hangar->grab = v3_sub(prop->entity->position, hit);
}

static void drag(Hangar *hangar, World *world, Ray ray)
{
    Prop *prop = &world->props[hangar->dragged];
    Vec3 hit;

    if (!picking_ground(ray, hangar->plane, &hit)) {
        return;
    }
    prop->entity->position = v3_add(hit, hangar->grab);
    push_clear(hangar, world, hangar->dragged);
}

static int free_pylon_near(const Hangar *hangar, const World *world, Vec3 position)
{
    float best = SNAP_REACH;
    int found = -1;
    int i;

    for (i = 0; i < WORLD_PYLONS; i++) {
        const Vec3 pylon = world_on_jet(world, world->pylons[i]);
        // the drag runs at whatever height the missile was taken from, so only the ground distance counts
        const float reach = v3_length(v3(pylon.x - position.x, 0.0f, pylon.z - position.z));

        if (hangar->mounted[i] < 0 && reach < best) {
            best = reach;
            found = i;
        }
    }

    return found;
}

static void let_go(Hangar *hangar, World *world)
{
    Prop *prop = &world->props[hangar->dragged];
    const int pylon = prop->kind == PROP_MISSILE ? free_pylon_near(hangar, world, prop->entity->position) : -1;

    if (pylon >= 0) {
        hangar->mounted[pylon] = hangar->dragged;
    } else {
        prop->entity->position.y = prop->rest;
    }
    hangar->dragged = -1;
}

static void untint(World *world)
{
    int i;

    for (i = 0; i < world->prop_count; i++) {
        world->props[i].entity->highlight = 0.0f;
    }
}

static void tint(World *world, int index, float strength)
{
    Entity *entity = world->props[index].entity;

    entity->highlight_color = tint_color;
    entity->highlight = strength;
}

void hangar_init(Hangar *hangar)
{
    int i;

    hangar->hovered = -1;
    hangar->dragged = -1;
    hangar->held = 0;
    for (i = 0; i < WORLD_PYLONS; i++) {
        hangar->mounted[i] = -1;
        hangar->takeoff[i] = -1;
    }
}

void hangar_release(Hangar *hangar, World *world)
{
    if (hangar->dragged >= 0) {
        let_go(hangar, world);
    }
    hangar->hovered = -1;
    hangar->held = 0;
    untint(world);
}

void hangar_takeoff(Hangar *hangar)
{
    int i;

    for (i = 0; i < WORLD_PYLONS; i++) {
        hangar->takeoff[i] = hangar->mounted[i];
    }
}

void hangar_step(Hangar *hangar, World *world, Ray ray, int button)
{
    if (hangar->dragged >= 0) {
        if (button) {
            drag(hangar, world, ray);
        } else {
            let_go(hangar, world);
        }
    } else {
        hangar->hovered = pick(world, ray);
        // only the step the button goes down on takes hold, so sliding onto a prop with it held picks nothing up
        if (button && !hangar->held && hangar->hovered >= 0) {
            take_hold(hangar, world, ray, hangar->hovered);
        }
    }
    hangar->held = button;

    untint(world);
    if (hangar->dragged >= 0) {
        tint(world, hangar->dragged, GRAB_TINT);
    } else if (hangar->hovered >= 0) {
        tint(world, hangar->hovered, HOVER_TINT);
    }
    hangar_carry(hangar, world);
}

void hangar_carry(const Hangar *hangar, World *world)
{
    int i;

    for (i = 0; i < WORLD_PYLONS; i++) {
        if (hangar->mounted[i] >= 0) {
            Entity *entity = world->props[hangar->mounted[i]].entity;

            entity->position = world_on_jet(world, world->pylons[i]);
            entity->orientation = world->jet->orientation;
        }
    }
}

int hangar_missiles(const Hangar *hangar)
{
    int count = 0;
    int i;

    for (i = 0; i < WORLD_PYLONS; i++) {
        count += hangar->mounted[i] >= 0;
    }

    return count;
}

// a pylon that took off loaded and now has nothing mounted fired its missile away this sortie
static int fired_since_takeoff(const Hangar *hangar, int pylon)
{
    return hangar->takeoff[pylon] >= 0 && hangar->mounted[pylon] < 0;
}

void hangar_restock_pylons(Hangar *hangar, World *world)
{
    int i;

    for (i = 0; i < WORLD_PYLONS; i++) {
        if (fired_since_takeoff(hangar, i)) {
            hangar->mounted[i] = hangar->takeoff[i];
            world->props[hangar->takeoff[i]].entity->hidden = 0;
        }
    }
}

void hangar_restock_cart(Hangar *hangar, World *world)
{
    int i;

    for (i = 0; i < WORLD_PYLONS; i++) {
        if (fired_since_takeoff(hangar, i)) {
            Prop *prop = &world->props[hangar->takeoff[i]];

            prop->entity->position = prop->home;
            prop->entity->orientation = quat_from_axis_angle(v3(0.0f, 1.0f, 0.0f), prop->yaw);
            prop->entity->hidden = 0;
        }
    }
}
