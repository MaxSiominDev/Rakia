#ifndef NOISE_H
#define NOISE_H

// 2D simplex noise: a seed picks one field, and a point always gives the same value in it on every platform;
// noise2 and noise_fbm stay within [-1, 1], noise_ridged within [0, 1] with 1 along the crests
float noise2(float x, float y, unsigned int seed);
float noise_fbm(float x, float y, unsigned int seed, int octaves, float lacunarity, float gain);
float noise_ridged(float x, float y, unsigned int seed, int octaves, float lacunarity, float gain);
// moves the sample point along the field itself, the usual cure for the grid look of plain fbm
void noise_warp(float *x, float *y, float frequency, float amount, unsigned int seed);
// a stream of values in [0, 1) carried in state, so a seed always gives the same sequence
float noise_random(unsigned int *state);

#endif
