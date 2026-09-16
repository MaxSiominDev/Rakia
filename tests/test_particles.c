#include "check.h"
#include "engine/particles.h"

#include <string.h>

static void test_spawn_dead_slot(void)
{
    Particles particles;
    Particle *revived;
    int i;

    memset(&particles, 0, sizeof particles);
    for (i = 0; i < PARTICLES_MAX; i++) {
        Particle *particle = particles_spawn(&particles);

        particle->lifetime = 10.0f;
        particle->age = 5.0f;
    }
    particles.pool[7].lifetime = 0.0f;
    particles.pool[900].age = 9.99f;

    revived = particles_spawn(&particles);
    check(revived == &particles.pool[7], "a dead slot is taken before any live one, however close to its end");
}

static void test_spawn_oldest(void)
{
    Particles particles;
    Particle *target;
    int i;

    memset(&particles, 0, sizeof particles);
    for (i = 0; i < PARTICLES_MAX; i++) {
        Particle *particle = particles_spawn(&particles);

        particle->lifetime = 10.0f;
    }
    target = &particles.pool[321];
    target->age = 9.9f;

    check(particles_spawn(&particles) == target, "a full pool gives up the slot nearest the end of its life");
}

static void test_step_updates(void)
{
    Particles particles;
    Particle *particle;

    memset(&particles, 0, sizeof particles);
    particle = particles_spawn(&particles);
    particle->lifetime = 1.0f;
    particle->velocity = v3(2.0f, 0.0f, 0.0f);
    particle->growth = 4.0f;
    particle->spin = 1.0f;

    particles_step(&particles, 0.5f);
    check(particle->lifetime > 0.0f, "halfway through its life a particle is still alive");
    check_close(particle->age, 0.5f, 1e-6f, "and has aged by the step");
    check_v3(particle->position, 1.0f, 0.0f, 0.0f, "moved by its velocity");
    check_close(particle->size, 2.0f, 1e-6f, "grown by its rate");
    check_close(particle->rotation, 0.5f, 1e-6f, "and spun by its own rate");

    particles_step(&particles, 0.6f);
    check(particle->lifetime <= 0.0f, "past its lifetime it dies");
}

static void test_order_alpha(void)
{
    Particles particles;
    int order[PARTICLES_MAX];
    Particle *near_sprite;
    Particle *far_sprite;
    Particle *middle_sprite;
    int count;

    memset(&particles, 0, sizeof particles);
    near_sprite = particles_spawn(&particles);
    near_sprite->lifetime = 1.0f;
    near_sprite->position = v3(0.0f, 0.0f, 1.0f);
    near_sprite->blend = PARTICLE_ALPHA;

    far_sprite = particles_spawn(&particles);
    far_sprite->lifetime = 1.0f;
    far_sprite->position = v3(0.0f, 0.0f, 10.0f);
    far_sprite->blend = PARTICLE_ALPHA;

    middle_sprite = particles_spawn(&particles);
    middle_sprite->lifetime = 1.0f;
    middle_sprite->position = v3(0.0f, 0.0f, 5.0f);
    middle_sprite->blend = PARTICLE_ALPHA;

    count = particles_order(&particles, v3(0.0f, 0.0f, 0.0f), order);
    check(count == 3, "every live particle is drawn");
    check(&particles.pool[order[0]] == far_sprite, "the furthest alpha sprite is drawn first");
    check(&particles.pool[order[1]] == middle_sprite, "then the middle one");
    check(&particles.pool[order[2]] == near_sprite, "and the nearest one last, so it composes over the rest");
}

static void test_order_additive(void)
{
    Particles particles;
    int order[PARTICLES_MAX];
    Particle *alpha;
    Particle *sheet2a;
    Particle *sheet1;
    Particle *sheet2b;
    int count;

    memset(&particles, 0, sizeof particles);
    alpha = particles_spawn(&particles);
    alpha->lifetime = 1.0f;
    alpha->blend = PARTICLE_ALPHA;

    sheet2a = particles_spawn(&particles);
    sheet2a->lifetime = 1.0f;
    sheet2a->blend = PARTICLE_ADDITIVE;
    sheet2a->sheet = 2;

    sheet1 = particles_spawn(&particles);
    sheet1->lifetime = 1.0f;
    sheet1->blend = PARTICLE_ADDITIVE;
    sheet1->sheet = 1;

    sheet2b = particles_spawn(&particles);
    sheet2b->lifetime = 1.0f;
    sheet2b->blend = PARTICLE_ADDITIVE;
    sheet2b->sheet = 2;

    count = particles_order(&particles, v3(0.0f, 0.0f, 0.0f), order);
    check(count == 4, "every live particle is drawn");
    check(&particles.pool[order[0]] == alpha, "the alpha sprite is drawn before any additive one");
    check(particles.pool[order[1]].sheet == 1, "additive sprites are grouped by sheet, the lower one first");
    check(particles.pool[order[2]].sheet == 2 && particles.pool[order[3]].sheet == 2,
          "so the two sharing a sheet land next to each other");
    check((&particles.pool[order[2]] == sheet2a && &particles.pool[order[3]] == sheet2b) ||
              (&particles.pool[order[2]] == sheet2b && &particles.pool[order[3]] == sheet2a),
          "and they are exactly the two spawned on it");
}

static void test_frame_from_age(void)
{
    Particles particles;
    Particle *particle;

    memset(&particles, 0, sizeof particles);
    particles.sheets[0].frames = 9;
    particles.sheets[0].columns = 3;
    particles.sheets[0].rows = 3;

    particle = particles_spawn(&particles);
    particle->sheet = 0;
    particle->frame = 0;
    particle->frame_rate = 9.0f;

    particle->age = 0.0f;
    check(particles_frame(&particles, particle) == 0, "a fresh sprite opens on its start frame");

    particle->age = 0.5f;
    check(particles_frame(&particles, particle) == 4, "and reads the frame off its age at the sheet's rate");

    particle->age = 10.0f;
    check(particles_frame(&particles, particle) == 8, "clamped to the sheet's last frame once the age runs past it");

    particle->frame = 3;
    particle->frame_rate = 0.0f;
    particle->age = 5.0f;
    check(particles_frame(&particles, particle) == 3, "a zero frame rate holds the frame it opened on");
}

void test_particles_main(void)
{
    test_spawn_dead_slot();
    test_spawn_oldest();
    test_step_updates();
    test_order_alpha();
    test_order_additive();
    test_frame_from_age();
}
