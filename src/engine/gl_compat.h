#ifndef GL_COMPAT_H
#define GL_COMPAT_H

#ifdef __APPLE__

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#else

#include <GL/freeglut.h>
#include <GL/glext.h>

#endif

#endif
