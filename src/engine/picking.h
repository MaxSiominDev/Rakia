#ifndef PICKING_H
#define PICKING_H

#include "engine/vecmath.h"

typedef struct {
    Vec3 origin;
    Vec3 direction;
} Ray;

// the cursor arrives in window points while the framebuffer may hold two pixels per point, so the ray is
// built from the point coordinates over the window size in points, y counted down from the top as GLUT
// reports it
Ray picking_ray(Mat4 view_projection, float x, float y, float width, float height);
// the box is given in the model's own frame and placed by model; distance is along the ray to where it enters
int picking_box(Ray ray, Mat4 model, Vec3 min, Vec3 max, float *distance);
// the horizontal plane at that height; misses when the ray runs away from it or along it
int picking_ground(Ray ray, float height, Vec3 *hit);
// where a world point lands on the screen, in the point coordinates picking_ray takes; 0 when it is behind
// the eye and nothing is written
int picking_screen(Mat4 view_projection, Vec3 point, float width, float height, float *x, float *y);

#endif
