#ifndef CHECK_H
#define CHECK_H

#include "engine/vecmath.h"

extern int failures;

void check(int condition, const char *what);
void check_close(float actual, float expected, float tolerance, const char *what);
void check_v3(Vec3 actual, float x, float y, float z, const char *what);

#endif
