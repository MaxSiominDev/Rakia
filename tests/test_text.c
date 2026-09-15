#include "check.h"
#include "engine/text.h"

#include <stdio.h>

#define FIXTURE "tests/fixtures/font_metrics.txt"
#define GENERATED "assets/generated/hud_font.txt"
// the advance of the question mark in the fixture, which every unknown code falls back to
#define FALLBACK_ADVANCE 5.0f

static char metrics[8192];
static Font font;
static Text text;

static int read_metrics(const char *path)
{
    FILE *file = fopen(path, "rb");
    size_t length = 0;

    if (file != NULL) {
        length = fread(metrics, 1, sizeof metrics - 1, file);
        fclose(file);
    }
    metrics[length] = '\0';

    return (int)length;
}

static void test_parse(void)
{
    const Glyph *a = &font.glyphs['A' - TEXT_FIRST_CODE];

    check(read_metrics(FIXTURE) > 0, "the metrics fixture is there");
    check(font_parse(&font, metrics) == 0, "it parses over its comment and its blank line");
    check(font.atlas_width == 64 && font.atlas_height == 32, "the atlas is 64 by 32");
    check(font.size == 10 && font.line_height == 14, "drawn at 10 px on a 14 px line");
    check(font.solid_x == 2 && font.solid_y == 2, "the solid texel is where the header puts it");
    check(a->x == 1 && a->y == 1 && a->width == 6 && a->height == 8, "A sits where its line puts it");
    check(a->bearing_x == 0 && a->bearing_y == 3 && a->advance == 7, "with its bearing and advance");
    check(font.glyphs['C' - TEXT_FIRST_CODE].advance == 0, "a code the file never named stays empty");
}

static void test_width(void)
{
    check_close(font_width(&font, "AB"), 14.0f, 0.001f, "a string is as wide as its advances");
    check_close(font_width(&font, "A B"), 18.0f, 0.001f, "the space carries its own advance");
    check_close(font_width(&font, ""), 0.0f, 0.001f, "the empty string has no width");
    check_close(font_width(&font, "C"), FALLBACK_ADVANCE, 0.001f, "a code the file never named falls back to ?");
    check_close(font_width(&font, "\x01"), FALLBACK_ADVANCE, 0.001f, "and so does a code outside the table");
}

static void test_layout(void)
{
    const Glyph *a = &font.glyphs['A' - TEXT_FIRST_CODE];
    float fallback_u;

    text.font = font;
    text_begin(&text, 800, 600);
    text_string(&text, 10.0f, 20.0f, 1.0f, "A");
    check(text.vertex_count == 6, "a glyph is one quad");
    check_close(text.vertices[0].x, 10.0f + (float)a->bearing_x, 0.001f, "the pen and the bearing place it");
    check_close(text.vertices[0].y, 20.0f + (float)a->bearing_y, 0.001f, "below the top of the line box");
    text_string(&text, 10.0f, 40.0f, 1.0f, " ");
    check(text.vertex_count == 6, "a space draws nothing");

    text_begin(&text, 800, 600);
    text_string(&text, 0.0f, 0.0f, 1.0f, "C");
    fallback_u = text.vertices[0].u;
    text_begin(&text, 800, 600);
    text_string(&text, 0.0f, 0.0f, 1.0f, "?");
    check_close(fallback_u, text.vertices[0].u, 0.001f, "an unknown code is drawn as a ?");
}

static void test_malformed(void)
{
    Font broken;

    check(font_parse(&broken, "# nothing but a comment\n\n") == 0, "comments and blank lines alone are a file");
    check(font_parse(&broken, "# note\r\natlas 64 32\r\nsize 10\r\n") == 0,
          "and the carriage returns a Windows checkout leaves are no obstacle");
    check(font_parse(&broken, "atlas 64\n") == -1, "a header line short of a number fails");
    check(font_parse(&broken, "glyph 65 1 1 6 8 0 3\n") == -1, "a glyph line short of a number fails");
    check(font_parse(&broken, "glyph 200 1 1 6 8 0 3 7\n") == -1, "a glyph code outside the table fails");
}

static void test_generated(void)
{
    Font generated;
    int complete = 1;
    int code;

    check(read_metrics(GENERATED) > 0, "the generated metrics are there");
    check(font_parse(&generated, metrics) == 0, "and they parse");
    for (code = TEXT_FIRST_CODE; code <= TEXT_LAST_CODE; code++) {
        complete &= generated.glyphs[code - TEXT_FIRST_CODE].advance > 0;
    }
    check(complete, "with every printable code in them");
    check(generated.solid_x > 0 && generated.solid_x < generated.atlas_width && generated.solid_y > 0 &&
          generated.solid_y < generated.atlas_height, "and a solid texel inside the atlas");
    // the hud divides by both of these to turn a point size into a scale
    check(generated.size > 0 && generated.line_height >= generated.size, "a pixel size, and a line to fit it in");
}

void test_text_main(void)
{
    test_parse();
    test_width();
    test_layout();
    test_malformed();
    test_generated();
}
