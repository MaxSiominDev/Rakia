#include "check.h"

#include <math.h>
#include <stdio.h>

int failures;

void check(int condition, const char *what)
{
    if (!condition) {
        printf("FAIL %s\n", what);
        failures++;
    }
}

void check_close(float actual, float expected, float tolerance, const char *what)
{
    if (fabsf(actual - expected) > tolerance) {
        printf("FAIL %s: got %.6f, want %.6f\n", what, actual, expected);
        failures++;
    }
}

void check_v3(Vec3 actual, float x, float y, float z, const char *what)
{
    check_close(actual.x, x, 1e-5f, what);
    check_close(actual.y, y, 1e-5f, what);
    check_close(actual.z, z, 1e-5f, what);
}

void test_vecmath_main(void);
void test_options_main(void);
void test_timestep_main(void);
void test_input_main(void);
void test_assets_main(void);
void test_mesh_data_main(void);
void test_camera_main(void);
void test_picking_main(void);
void test_collision_main(void);
void test_noise_main(void);
void test_terrain_main(void);
void test_foliage_main(void);
void test_gl_ext_main(void);
void test_light_main(void);
void test_shadow_main(void);
void test_post_main(void);
void test_scene_main(void);
void test_obj_main(void);
void test_image_main(void);
void test_text_main(void);
void test_aircraft_main(void);
void test_takeoff_main(void);
void test_hangar_main(void);
void test_particles_main(void);
void test_effects_main(void);
void test_targets_main(void);
void test_weapons_main(void);
void test_game_main(void);

int main(void)
{
    test_vecmath_main();
    test_options_main();
    test_timestep_main();
    test_input_main();
    test_assets_main();
    test_mesh_data_main();
    test_camera_main();
    test_picking_main();
    test_collision_main();
    test_noise_main();
    test_terrain_main();
    test_foliage_main();
    test_gl_ext_main();
    test_light_main();
    test_shadow_main();
    test_post_main();
    test_scene_main();
    test_obj_main();
    test_image_main();
    test_text_main();
    test_aircraft_main();
    test_takeoff_main();
    test_hangar_main();
    test_particles_main();
    test_effects_main();
    test_targets_main();
    test_weapons_main();
    test_game_main();

    if (failures > 0) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }

    printf("all checks passed\n");
    return 0;
}
