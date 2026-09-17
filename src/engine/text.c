#include "engine/text.h"

#include "engine/assets.h"
#include "engine/gl_ext.h"
#include "engine/material.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define METRICS_FILE "generated/hud_font.txt"
#define ATLAS_FILE "generated/hud_font.png"
#define LINE_CAPACITY 128

static int parse_line(Font *font, const char *line)
{
    int values[8];

    if (sscanf(line, "atlas %d %d", &values[0], &values[1]) == 2) {
        font->atlas_width = values[0];
        font->atlas_height = values[1];
        return 0;
    }
    if (sscanf(line, "size %d", &values[0]) == 1) {
        font->size = values[0];
        return 0;
    }
    if (sscanf(line, "line %d", &values[0]) == 1) {
        font->line_height = values[0];
        return 0;
    }
    if (sscanf(line, "solid %d %d", &values[0], &values[1]) == 2) {
        font->solid_x = values[0];
        font->solid_y = values[1];
        return 0;
    }
    if (sscanf(line, "glyph %d %d %d %d %d %d %d %d", &values[0], &values[1], &values[2], &values[3], &values[4],
               &values[5], &values[6], &values[7]) == 8 &&
        values[0] >= TEXT_FIRST_CODE && values[0] <= TEXT_LAST_CODE) {
        Glyph *glyph = &font->glyphs[values[0] - TEXT_FIRST_CODE];

        glyph->x = (short)values[1];
        glyph->y = (short)values[2];
        glyph->width = (short)values[3];
        glyph->height = (short)values[4];
        glyph->bearing_x = (short)values[5];
        glyph->bearing_y = (short)values[6];
        glyph->advance = (short)values[7];
        return 0;
    }

    return -1;
}

int font_parse(Font *font, const char *text)
{
    const char *line = text;
    int number;

    memset(font, 0, sizeof *font);
    for (number = 1; *line != '\0'; number++) {
        const char *end = strchr(line, '\n');
        const size_t length = end != NULL ? (size_t)(end - line) : strlen(line);
        char copy[LINE_CAPACITY];
        const char *content;

        if (length >= sizeof copy) {
            fprintf(stderr, "cannot parse the font metrics at line %d: the line is too long\n", number);
            return -1;
        }
        memcpy(copy, line, length);
        copy[length] = '\0';
        line += end != NULL ? length + 1 : length;
        content = copy + strspn(copy, " \t\r");
        if (*content != '\0' && *content != '#' && parse_line(font, content) != 0) {
            fprintf(stderr, "cannot parse the font metrics at line %d: %s\n", number, copy);
            return -1;
        }
    }

    return 0;
}

// a code the metrics did not carry keeps a zero advance, so it falls back the way a code out of range does
static const Glyph *font_glyph(const Font *font, char code)
{
    const int index = (unsigned char)code - TEXT_FIRST_CODE;

    if (index < 0 || index >= TEXT_CODE_COUNT || font->glyphs[index].advance == 0) {
        return &font->glyphs['?' - TEXT_FIRST_CODE];
    }

    return &font->glyphs[index];
}

float font_width(const Font *font, const char *string)
{
    float width = 0.0f;
    int i;

    for (i = 0; string[i] != '\0'; i++) {
        width += (float)font_glyph(font, string[i])->advance;
    }

    return width;
}

// the corners of a quad, and the winding that still faces the front once the projection flips y
static const float corner_uvs[4][2] = {{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}};
static const int corner_order[6] = {0, 1, 2, 0, 2, 3};

// the corners run around the quad and the source rectangle is in atlas pixels, from its top left corner
static void push_corners(Text *text, const float corners[4][2], float sx, float sy, float swidth, float sheight)
{
    const float to_u = 1.0f / (float)text->font.atlas_width;
    const float to_v = 1.0f / (float)text->font.atlas_height;
    TextVertex *vertex;
    int i;

    if (text->vertex_count + 6 > TEXT_QUAD_MAX * 6) {
        return;
    }
    vertex = &text->vertices[text->vertex_count];
    for (i = 0; i < 6; i++) {
        const int corner = corner_order[i];

        vertex[i].x = corners[corner][0];
        vertex[i].y = corners[corner][1];
        vertex[i].u = (sx + swidth * corner_uvs[corner][0]) * to_u;
        // image.c flips the png as it loads, so the top row of the atlas is the last row of the texture
        vertex[i].v = 1.0f - (sy + sheight * corner_uvs[corner][1]) * to_v;
    }
    text->vertex_count += 6;
    text->runs[text->run_count - 1].count += 6;
}

static void push_quad(Text *text, float x, float y, float width, float height,
                      float sx, float sy, float swidth, float sheight)
{
    const float corners[4][2] = {{x, y}, {x, y + height}, {x + width, y + height}, {x + width, y}};

    push_corners(text, corners, sx, sy, swidth, sheight);
}

static void push_solid(Text *text, const float corners[4][2])
{
    push_corners(text, corners, (float)text->font.solid_x + 0.5f, (float)text->font.solid_y + 0.5f, 0.0f, 0.0f);
}

void text_begin(Text *text, int width, int height)
{
    text->projection = m4_orthographic(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    text->vertex_count = 0;
    text->run_count = 1;
    text->runs[0].color = v3(1.0f, 1.0f, 1.0f);
    text->runs[0].first = 0;
    text->runs[0].count = 0;
}

void text_color(Text *text, Vec3 color)
{
    TextRun *run = &text->runs[text->run_count - 1];

    if (run->color.x == color.x && run->color.y == color.y && run->color.z == color.z) {
        return;
    }
    if (run->count == 0) {
        run->color = color;
        return;
    }
    if (text->run_count == TEXT_RUN_MAX) {
        return;
    }
    run = &text->runs[text->run_count++];
    run->color = color;
    run->first = text->vertex_count;
    run->count = 0;
}

void text_string(Text *text, float x, float y, float scale, const char *string)
{
    float pen = x;
    int i;

    for (i = 0; string[i] != '\0'; i++) {
        const Glyph *glyph = font_glyph(&text->font, string[i]);

        if (glyph->width > 0) {
            push_quad(text, pen + (float)glyph->bearing_x * scale, y + (float)glyph->bearing_y * scale,
                      (float)glyph->width * scale, (float)glyph->height * scale,
                      (float)glyph->x, (float)glyph->y, (float)glyph->width, (float)glyph->height);
        }
        pen += (float)glyph->advance * scale;
    }
}

void text_quad(Text *text, float x, float y, float width, float height)
{
    const float corners[4][2] = {{x, y}, {x, y + height}, {x + width, y + height}, {x + width, y}};

    push_solid(text, corners);
}

void text_line(Text *text, float x0, float y0, float x1, float y1, float thickness)
{
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float length = sqrtf(dx * dx + dy * dy);
    // half the thickness across the line, so the bar is centered on it
    const float across_x = length > 0.0f ? -dy / length * thickness * 0.5f : 0.0f;
    const float across_y = length > 0.0f ? dx / length * thickness * 0.5f : 0.0f;
    const float corners[4][2] = {{x0 - across_x, y0 - across_y}, {x0 + across_x, y0 + across_y},
                                 {x1 + across_x, y1 + across_y}, {x1 - across_x, y1 - across_y}};

    push_solid(text, corners);
}

void text_end(Text *text)
{
    int i;

    if (text->vertex_count == 0) {
        return;
    }
    glUseProgram(text->shader.program);
    shader_set_mat4(&text->shader, "u_projection", text->projection);
    glBindBuffer(GL_ARRAY_BUFFER, text->buffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof *text->vertices * (size_t)text->vertex_count),
                 text->vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(SHADER_ATTRIBUTE_POSITION);
    glEnableVertexAttribArray(SHADER_ATTRIBUTE_UV);
    glVertexAttribPointer(SHADER_ATTRIBUTE_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
                          (const void *)(uintptr_t)offsetof(TextVertex, x));
    glVertexAttribPointer(SHADER_ATTRIBUTE_UV, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
                          (const void *)(uintptr_t)offsetof(TextVertex, u));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, text->atlas);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    for (i = 0; i < text->run_count; i++) {
        shader_set_vec3(&text->shader, "u_color", text->runs[i].color);
        glDrawArrays(GL_TRIANGLES, text->runs[i].first, text->runs[i].count);
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_UV);
    glDisableVertexAttribArray(SHADER_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

int text_init(Text *text)
{
    unsigned char *metrics;
    size_t metrics_size;
    const char *reason;

    memset(text, 0, sizeof *text);
    if (assets_read(METRICS_FILE, &metrics, &metrics_size, &reason) != 0) {
        fprintf(stderr, "cannot open %s: %s\n", METRICS_FILE, reason);
        return -1;
    }
    if (font_parse(&text->font, (const char *)metrics) != 0) {
        free(metrics);
        return -1;
    }
    free(metrics);
    if (shader_load(&text->shader, "hud") != 0) {
        return -1;
    }
    text->atlas = material_texture(ATLAS_FILE, TEXTURE_SCREEN);
    if (text->atlas == 0) {
        return -1;
    }

    glGenBuffers(1, &text->buffer);
    glUseProgram(text->shader.program);
    shader_set_int(&text->shader, "u_atlas", 0);
    glUseProgram(0);

    return 0;
}
