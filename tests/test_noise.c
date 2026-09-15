#include "check.h"
#include "engine/noise.h"

#include <math.h>

#define SAMPLES 64
#define SEED 61873u
#define OCTAVES 5
#define LACUNARITY 2.0f
#define GAIN 0.5f
#define STEP 0.001f
// the worst step of this size measured over the grid below moves noise2 by about 0.007
#define STEP_BOUND 0.02f
#define WARP_FREQUENCY 0.05f
#define WARP_AMOUNT 4.0f
#define MOST (SAMPLES * SAMPLES * 9 / 10)

static float coordinate(int index)
{
    return -9.7f + (float)index * 0.3137f;
}

static void test_range(void)
{
    float lowest = 1.0f;
    float highest = -1.0f;
    int noise_in_range = 1;
    int fbm_in_range = 1;
    int ridged_in_range = 1;
    int ix;
    int iy;

    for (ix = 0; ix < SAMPLES; ix++) {
        for (iy = 0; iy < SAMPLES; iy++) {
            const float x = coordinate(ix);
            const float y = coordinate(iy);
            const float n = noise2(x, y, SEED);
            const float f = noise_fbm(x, y, SEED, OCTAVES, LACUNARITY, GAIN);
            const float r = noise_ridged(x, y, SEED, OCTAVES, LACUNARITY, GAIN);

            noise_in_range &= n >= -1.0f && n <= 1.0f;
            fbm_in_range &= f >= -1.0f && f <= 1.0f;
            ridged_in_range &= r >= 0.0f && r <= 1.0f;
            lowest = n < lowest ? n : lowest;
            highest = n > highest ? n : highest;
        }
    }

    check(noise_in_range, "noise2 stays within [-1, 1]");
    check(fbm_in_range, "fbm stays within [-1, 1]");
    check(ridged_in_range, "ridged stays within [0, 1]");
    check(lowest < -0.5f && highest > 0.5f, "noise2 uses most of its range");
}

static void test_repeatability(void)
{
    int noise_repeats = 1;
    int fbm_repeats = 1;
    int other_seed_differs = 0;
    int ix;
    int iy;

    for (ix = 0; ix < SAMPLES; ix++) {
        for (iy = 0; iy < SAMPLES; iy++) {
            const float x = coordinate(ix);
            const float y = coordinate(iy);
            const float n = noise2(x, y, SEED);
            const float f = noise_fbm(x, y, SEED, OCTAVES, LACUNARITY, GAIN);

            noise_repeats &= noise2(x, y, SEED) == n;
            fbm_repeats &= noise_fbm(x, y, SEED, OCTAVES, LACUNARITY, GAIN) == f;
            other_seed_differs += noise2(x, y, SEED + 1u) != n;
        }
    }

    check(noise_repeats, "noise2 repeats bit for bit");
    check(fbm_repeats, "fbm repeats bit for bit");
    check(other_seed_differs > MOST, "another seed gives another field");
}

static void test_continuity(void)
{
    float worst = 0.0f;
    int ix;
    int iy;

    for (ix = 0; ix < SAMPLES; ix++) {
        for (iy = 0; iy < SAMPLES; iy++) {
            const float x = coordinate(ix);
            const float y = coordinate(iy);
            const float n = noise2(x, y, SEED);
            const float along_x = fabsf(noise2(x + STEP, y, SEED) - n);
            const float along_y = fabsf(noise2(x, y + STEP, SEED) - n);

            worst = along_x > worst ? along_x : worst;
            worst = along_y > worst ? along_y : worst;
        }
    }

    check(worst < STEP_BOUND, "a tiny step barely moves noise2");
}

static void test_octaves(void)
{
    const float x = 3.25f;
    const float y = -7.5f;

    check(noise_fbm(x, y, SEED, 1, LACUNARITY, GAIN) == noise2(x, y, SEED), "one octave is plain noise2");
    check(noise_fbm(x, y, SEED, 0, LACUNARITY, GAIN) == 0.0f, "fbm without octaves is flat");
    check(noise_ridged(x, y, SEED, 0, LACUNARITY, GAIN) == 0.0f, "ridged without octaves is flat");
    // octaves at the same frequency and amplitude would cancel out to plain noise2 if they shared a seed
    check(noise_fbm(x, y, SEED, 4, 1.0f, 1.0f) != noise2(x, y, SEED), "every octave gets its own field");
}

static void test_warp(void)
{
    int untouched = 1;
    int bounded = 1;
    int moved = 0;
    int axes_differ = 0;
    int ix;
    int iy;

    for (ix = 0; ix < SAMPLES; ix++) {
        for (iy = 0; iy < SAMPLES; iy++) {
            const float x = coordinate(ix);
            const float y = coordinate(iy);
            float kept_x = x;
            float kept_y = y;
            float warped_x = x;
            float warped_y = y;

            noise_warp(&kept_x, &kept_y, WARP_FREQUENCY, 0.0f, SEED);
            noise_warp(&warped_x, &warped_y, WARP_FREQUENCY, WARP_AMOUNT, SEED);

            untouched &= kept_x == x && kept_y == y;
            bounded &= fabsf(warped_x - x) <= WARP_AMOUNT && fabsf(warped_y - y) <= WARP_AMOUNT;
            moved += warped_x != x || warped_y != y;
            axes_differ += (warped_x - x) != (warped_y - y);
        }
    }

    check(untouched, "a zero amount leaves the point alone");
    check(bounded, "the warp moves a point by at most the amount on each axis");
    check(moved > MOST, "the warp does move points");
    check(axes_differ > MOST, "the two axes are warped by different fields");
}

void test_noise_main(void)
{
    test_range();
    test_repeatability();
    test_continuity();
    test_octaves();
    test_warp();
}
