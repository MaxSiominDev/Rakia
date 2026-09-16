#include "engine/noise.h"

#include <math.h>

// to and from the triangular lattice: (sqrt(3) - 1) / 2 and (3 - sqrt(3)) / 6
#define SKEW 0.36602540378f
#define UNSKEW 0.21132486540f
// squared reach of one corner's kernel
#define REACH 0.5f
// brings the sum of the three corners up to the full [-1, 1] without leaving it
#define SCALE 99.2f
// every octave and every warp axis gets a field of its own
#define OCTAVE_STRIDE 0x9e3779b9u
#define WARP_X_SEED 0x1b873593u
#define WARP_Y_SEED 0xcc9e2d51u

#define DIAGONAL 0.70710678f

static const float gradients[8][2] = {
    {1.0f, 0.0f}, {DIAGONAL, DIAGONAL}, {0.0f, 1.0f}, {-DIAGONAL, DIAGONAL},
    {-1.0f, 0.0f}, {-DIAGONAL, -DIAGONAL}, {0.0f, -1.0f}, {DIAGONAL, -DIAGONAL}
};

// murmur3 finalizer over the lattice cell and the seed, in wrapping 32-bit arithmetic
static unsigned int hash2(int i, int j, unsigned int seed)
{
    unsigned int h = (unsigned int)i * 0x9e3779b1u + (unsigned int)j * 0x85ebca77u + seed * 0xc2b2ae3du;

    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;

    return h;
}

static int floor_to_int(float v)
{
    const int truncated = (int)v;

    return v < (float)truncated ? truncated - 1 : truncated;
}

static float corner(float dx, float dy, int i, int j, unsigned int seed)
{
    const float falloff = REACH - dx * dx - dy * dy;
    const float *gradient;

    if (falloff <= 0.0f) {
        return 0.0f;
    }
    gradient = gradients[hash2(i, j, seed) & 7u];

    return falloff * falloff * falloff * falloff * (gradient[0] * dx + gradient[1] * dy);
}

static unsigned int octave_seed(unsigned int seed, int octave)
{
    return seed + (unsigned int)octave * OCTAVE_STRIDE;
}

float noise2(float x, float y, unsigned int seed)
{
    const float skew = (x + y) * SKEW;
    const int i = floor_to_int(x + skew);
    const int j = floor_to_int(y + skew);
    const float unskew = (float)(i + j) * UNSKEW;
    const float x0 = x - ((float)i - unskew);
    const float y0 = y - ((float)j - unskew);
    // the diagonal splits the skewed cell into a lower and an upper triangle
    const int i1 = x0 > y0 ? 1 : 0;
    const int j1 = x0 > y0 ? 0 : 1;
    const float sum = corner(x0, y0, i, j, seed) +
                      corner(x0 - (float)i1 + UNSKEW, y0 - (float)j1 + UNSKEW, i + i1, j + j1, seed) +
                      corner(x0 - 1.0f + 2.0f * UNSKEW, y0 - 1.0f + 2.0f * UNSKEW, i + 1, j + 1, seed);

    return sum * SCALE;
}

float noise_fbm(float x, float y, unsigned int seed, int octaves, float lacunarity, float gain)
{
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float sum = 0.0f;
    float total = 0.0f;
    int octave;

    for (octave = 0; octave < octaves; octave++) {
        sum += amplitude * noise2(x * frequency, y * frequency, octave_seed(seed, octave));
        total += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    return total > 0.0f ? sum / total : 0.0f;
}

float noise_ridged(float x, float y, unsigned int seed, int octaves, float lacunarity, float gain)
{
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float sum = 0.0f;
    float total = 0.0f;
    int octave;

    for (octave = 0; octave < octaves; octave++) {
        const float ridge = 1.0f - fabsf(noise2(x * frequency, y * frequency, octave_seed(seed, octave)));

        sum += amplitude * ridge * ridge;
        total += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    return total > 0.0f ? sum / total : 0.0f;
}

void noise_warp(float *x, float *y, float frequency, float amount, unsigned int seed)
{
    const float sx = *x * frequency;
    const float sy = *y * frequency;

    *x += amount * noise2(sx, sy, seed ^ WARP_X_SEED);
    *y += amount * noise2(sx, sy, seed ^ WARP_Y_SEED);
}

float noise_random(unsigned int *state)
{
    *state = *state * 1664525u + 1013904223u;

    return (float)(*state >> 8) * (1.0f / 16777216.0f);
}
