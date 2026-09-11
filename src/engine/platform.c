#include "engine/platform.h"

#include "engine/gl_ext.h"
#include "engine/hidpi.h"
#include "engine/screenshot.h"
#include "engine/timestep.h"

#include <stdio.h>
#include <stdlib.h>

// on macOS the first frames of a new window can read back garbage
#define SCREENSHOT_WARMUP_FRAMES 3
#define WINDOW_TITLE "Rakia"
#define DEFAULT_WINDOW_WIDTH 1280
#define DEFAULT_WINDOW_HEIGHT 720
#define TICK_INTERVAL_MS 1

static const Options *options;
static const PlatformApp *app;
static Input input;
static Timestep timestep;
static int viewport_width;
static int viewport_height;
static int windowed_width;
static int windowed_height;
static int fullscreen;
static int frames_rendered;
static int bench_started_at;
static int frame_requested;

static int save_screenshot(const char *path)
{
    glFinish();

    if (screenshot_save_bmp(path, viewport_width, viewport_height) != 0) {
        fprintf(stderr, "cannot write %s\n", path);
        return -1;
    }

    printf("wrote %s (%dx%d)\n", path, viewport_width, viewport_height);
    return 0;
}

static void request_frame(void)
{
    frame_requested = 1;
    glutPostRedisplay();
}

static void finish_benchmark(void)
{
    if (frames_rendered == SCREENSHOT_WARMUP_FRAMES) {
        bench_started_at = glutGet(GLUT_ELAPSED_TIME);
    }
    if (frames_rendered < SCREENSHOT_WARMUP_FRAMES + options->bench_frames) {
        request_frame();
        return;
    }

    printf("%d frames at %dx%d: %.2f ms/frame\n", options->bench_frames, viewport_width, viewport_height,
           (double)(glutGet(GLUT_ELAPSED_TIME) - bench_started_at) / options->bench_frames);
    exit(EXIT_SUCCESS);
}

static void display(void)
{
    // macOS GLUT runs this twice per request, so a call without a new frame is skipped
    if (!frame_requested || viewport_width <= 0 || viewport_height <= 0) {
        return;
    }
    frame_requested = 0;

    app->render(app->context, viewport_width, viewport_height);
    frames_rendered++;

    if (options->bench_frames > 0) {
        glFinish();
        finish_benchmark();
        return;
    }

    if (options->screenshot_path != NULL) {
        if (frames_rendered >= SCREENSHOT_WARMUP_FRAMES) {
            exit(save_screenshot(options->screenshot_path) == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
        }
        request_frame();
    }

    glutSwapBuffers();
}

static void tick(int unused)
{
    const int steps = timestep_advance(&timestep, glutGet(GLUT_ELAPSED_TIME));
    int i;

    (void)unused;
    for (i = 0; i < steps; i++) {
        app->update(app->context, &input, TIMESTEP_DT);
    }
    // macOS GLUT ignores the swap interval, so frames are paced by the simulation instead of vsync
    if (steps > 0) {
        request_frame();
    }
    glutTimerFunc(TICK_INTERVAL_MS, tick, 0);
}

static void reshape(int width, int height)
{
    const float scale = hidpi_scale();

    viewport_width = (int)((float)width * scale + 0.5f);
    viewport_height = (int)((float)height * scale + 0.5f);
    glViewport(0, 0, viewport_width, viewport_height);
    frame_requested = 1;
}

static void toggle_fullscreen(void)
{
    if (fullscreen) {
        // GLUT.framework has no glutLeaveFullScreen
#ifdef __APPLE__
        glutReshapeWindow(windowed_width, windowed_height);
#else
        glutLeaveFullScreen();
#endif
    } else {
        glutFullScreen();
    }
    fullscreen = !fullscreen;
}

static void keyboard(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;
    input_key(&input, key, 1);
}

static void keyboard_up(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;
    input_key(&input, key, 0);
}

static void special(int key, int x, int y)
{
    (void)x;
    (void)y;
    if (key == GLUT_KEY_F11) {
        toggle_fullscreen();
        return;
    }
    input_special(&input, key, 1);
}

static void special_up(int key, int x, int y)
{
    (void)x;
    (void)y;
    input_special(&input, key, 0);
}

static void mouse(int button, int state, int x, int y)
{
    input_mouse_move(&input, x, y);
    input_button(&input, button, state == GLUT_DOWN);
}

static void motion(int x, int y)
{
    input_mouse_move(&input, x, y);
}

int platform_run(const Options *run_options, const PlatformApp *run_app, int *argc, char **argv)
{
    const char *missing = NULL;
    int loaded;

    options = run_options;
    app = run_app;
    windowed_width = options->windowed ? options->window_width : DEFAULT_WINDOW_WIDTH;
    windowed_height = options->windowed ? options->window_height : DEFAULT_WINDOW_HEIGHT;

    glutInit(argc, argv);
    glutInitDisplayString("rgb double depth samples=4");
    glutInitWindowSize(windowed_width, windowed_height);
    glutCreateWindow(WINDOW_TITLE);
    hidpi_enable();

    printf("OpenGL %s on %s\n", (const char *)glGetString(GL_VERSION), (const char *)glGetString(GL_RENDERER));
    loaded = gl_ext_load(&missing) == 0;
    gl_ext_print_status();
    if (!loaded) {
        fprintf(stderr, "this OpenGL driver has no %s, which Rakia needs for shaders and vertex buffers\n", missing);
        return EXIT_FAILURE;
    }
    gl_ext_set_swap_interval(1);

    if (app->init(app->context) != 0) {
        return EXIT_FAILURE;
    }

    glutIgnoreKeyRepeat(1);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboard_up);
    glutSpecialFunc(special);
    glutSpecialUpFunc(special_up);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(motion);
    // a screenshot shows the initial state, so the simulation clock only runs in the other modes
    if (options->screenshot_path == NULL) {
        glutTimerFunc(TICK_INTERVAL_MS, tick, 0);
    }

    if (!options->windowed) {
        toggle_fullscreen();
    }

    timestep_init(&timestep, glutGet(GLUT_ELAPSED_TIME));
    glutMainLoop();

    return EXIT_SUCCESS;
}
