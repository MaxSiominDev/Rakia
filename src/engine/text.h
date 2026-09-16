#ifndef TEXT_H
#define TEXT_H

#include "engine/gl_compat.h"
#include "engine/shader.h"
#include "engine/vecmath.h"

#define TEXT_FIRST_CODE 32
#define TEXT_LAST_CODE 126
#define TEXT_CODE_COUNT (TEXT_LAST_CODE - TEXT_FIRST_CODE + 1)
// one frame's batch; quads and color changes past these are dropped
#define TEXT_QUAD_MAX 1024
#define TEXT_RUN_MAX 32

typedef struct {
    // place in the atlas, with the origin in its top left corner
    short x;
    short y;
    short width;
    short height;
    // from the pen to the top left corner of the glyph box
    short bearing_x;
    short bearing_y;
    short advance;
} Glyph;

typedef struct {
    Glyph glyphs[TEXT_CODE_COUNT];
    int atlas_width;
    int atlas_height;
    // the pixel size the glyphs were rasterized at, so scale 1 draws them as they were drawn
    int size;
    int line_height;
    // a fully opaque texel, so boxes and lines come from the same texture as the text
    int solid_x;
    int solid_y;
} Font;

// the metrics file held in memory; -1 and a message on stderr when a line is malformed
int font_parse(Font *font, const char *text);
// in atlas pixels, so the width at scale 1
float font_width(const Font *font, const char *string);

typedef struct {
    float x;
    float y;
    float u;
    float v;
} TextVertex;

typedef struct {
    Vec3 color;
    int first;
    int count;
} TextRun;

typedef struct {
    Font font;
    Shader shader;
    GLuint atlas;
    GLuint buffer;
    Mat4 projection;
    TextVertex vertices[TEXT_QUAD_MAX * 6];
    TextRun runs[TEXT_RUN_MAX];
    int vertex_count;
    int run_count;
} Text;

int text_init(Text *text);
// the framebuffer in pixels; every frame opens a new batch
void text_begin(Text *text, int width, int height);
void text_color(Text *text, Vec3 color);
// x, y is the top left corner of the line box, which reaches down to y + line_height * scale
void text_string(Text *text, float x, float y, float scale, const char *string);
void text_quad(Text *text, float x, float y, float width, float height);
// a bar of that thickness centered on the line between the two points, for the marks a HUD draws at an angle
void text_line(Text *text, float x0, float y0, float x1, float y1, float thickness);
void text_end(Text *text);

#endif
