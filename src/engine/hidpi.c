#include "engine/hidpi.h"

#ifdef __APPLE__

#include <objc/message.h>
#include <objc/runtime.h>

typedef id (*ObjectMessage)(id, SEL);
typedef void (*VoidMessage)(id, SEL);
typedef unsigned long (*CountMessage)(id, SEL);
typedef id (*IndexMessage)(id, SEL, unsigned long);
typedef void (*FlagMessage)(id, SEL, signed char);
typedef double (*DoubleMessage)(id, SEL);

static id glut_window;

static id send(id receiver, const char *selector)
{
    return ((ObjectMessage)objc_msgSend)(receiver, sel_registerName(selector));
}

void hidpi_enable(void)
{
    const id application = send((id)objc_getClass("NSApplication"), "sharedApplication");
    const id windows = send(application, "windows");
    const unsigned long count = ((CountMessage)objc_msgSend)(windows, sel_registerName("count"));
    const id context = send((id)objc_getClass("NSOpenGLContext"), "currentContext");
    unsigned long i;

    if (count == 0 || context == NULL) {
        return;
    }

    for (i = 0; i < count; i++) {
        glut_window = ((IndexMessage)objc_msgSend)(windows, sel_registerName("objectAtIndex:"), i);
        ((FlagMessage)objc_msgSend)(send(glut_window, "contentView"),
                                    sel_registerName("setWantsBestResolutionOpenGLSurface:"), 1);
    }
    // GLUT already created the surface at 1x, so the context has to pick up the new backing size
    ((VoidMessage)objc_msgSend)(context, sel_registerName("update"));
}

float hidpi_scale(void)
{
    if (glut_window == NULL) {
        return 1.0f;
    }

    return (float)((DoubleMessage)objc_msgSend)(glut_window, sel_registerName("backingScaleFactor"));
}

#else

void hidpi_enable(void)
{
}

float hidpi_scale(void)
{
    return 1.0f;
}

#endif
