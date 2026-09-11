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

void test_vecmath_main(void);
void test_options_main(void);
void test_timestep_main(void);
void test_input_main(void);
void test_assets_main(void);
void test_mesh_data_main(void);
void test_camera_main(void);
void test_gl_ext_main(void);

int main(void)
{
    test_vecmath_main();
    test_options_main();
    test_timestep_main();
    test_input_main();
    test_assets_main();
    test_mesh_data_main();
    test_camera_main();
    test_gl_ext_main();

    if (failures > 0) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }

    printf("all checks passed\n");
    return 0;
}
