#include "game/hud.h"

#include <math.h>
#include <stdio.h>

// the layout is written in points of a 720 point tall frame and scales with the window, so it reads the same
// on a Retina backing as on Windows
#define VALUE_POINTS 22.0f
#define LABEL_POINTS 13.0f
#define MESSAGE_POINTS 34.0f
#define HINT_POINTS 15.0f
#define MARGIN 44.0f
#define MESSAGE_TOP 0.34f
// the gun cross of a real HUD: four bars around a gap the nose sits in
#define MARKER_ARM 18.0f
#define MARKER_GAP 6.0f
#define MARKER_WEIGHT 2.0f

#define LOCK_WEIGHT 2.0f
#define DIAMOND_RADIUS 16.0f
#define DIAMOND_WEIGHT 2.0f
#define ARROW_LENGTH 16.0f
#define ARROW_SPREAD 9.0f
#define ARROW_WEIGHT 2.0f
#define LABEL_GAP 4.0f
#define PULL_UP_POINTS 30.0f
#define PULL_UP_Y_SHARE 0.24f
#define SHADOW_OFFSET 2.0f
#define NUMBER_LENGTH 32

static const Vec3 hud_color = {0.74f, 0.98f, 0.72f};
static const Vec3 hud_shadow = {0.03f, 0.07f, 0.03f};

typedef struct {
    Text *text;
    float width;
    float height;
    // pixels per point, and the pen offset that draws the shadow copy
    float scale;
    float offset;
} Layout;

// text_string scales the atlas, so a point size divides by the size the glyphs were rasterized at
static float glyph_scale(const Layout *layout, float size)
{
    return size / (float)layout->text->font.size * layout->scale;
}

static void line(const Layout *layout, float x, float y, float size, const char *string)
{
    text_string(layout->text, x + layout->offset, y + layout->offset, glyph_scale(layout, size), string);
}

static void right_aligned(const Layout *layout, float right, float y, float size, const char *string)
{
    line(layout, right - font_width(&layout->text->font, string) * glyph_scale(layout, size), y, size, string);
}

static void centered(const Layout *layout, float y, float size, const char *string)
{
    line(layout, (layout->width - font_width(&layout->text->font, string) * glyph_scale(layout, size)) * 0.5f, y,
         size, string);
}

static void bar(const Layout *layout, float x, float y, float width, float height)
{
    text_quad(layout->text, x + layout->offset, y + layout->offset, width, height);
}

static void stroke(const Layout *layout, float x0, float y0, float x1, float y1, float weight)
{
    text_line(layout->text, x0 + layout->offset, y0 + layout->offset, x1 + layout->offset, y1 + layout->offset,
             weight * layout->scale);
}

// where the camera looks, which is where the nose is pointing in the cockpit view
static void marker(const Layout *layout)
{
    const float arm = MARKER_ARM * layout->scale;
    const float gap = MARKER_GAP * layout->scale;
    const float weight = MARKER_WEIGHT * layout->scale;
    const float x = layout->width * 0.5f;
    const float y = layout->height * 0.5f;

    bar(layout, x - gap - arm, y - weight * 0.5f, arm, weight);
    bar(layout, x + gap, y - weight * 0.5f, arm, weight);
    bar(layout, x - weight * 0.5f, y - gap - arm, weight, arm);
    bar(layout, x - weight * 0.5f, y + gap, weight, arm);
}

static void readouts(const Layout *layout, const HudState *state)
{
    const float margin = MARGIN * layout->scale;
    const float label = layout->height * 0.5f - (VALUE_POINTS + LABEL_POINTS) * layout->scale;
    const float value = layout->height * 0.5f - VALUE_POINTS * layout->scale;
    char number[NUMBER_LENGTH];

    // a compass reads whole degrees and rolls 360 back to 000
    snprintf(number, sizeof number, "HDG %03d", (int)(state->heading + 0.5f) % 360);
    centered(layout, margin, VALUE_POINTS, number);

    line(layout, margin, label, LABEL_POINTS, "SPD");
    snprintf(number, sizeof number, "%.0f KM/H", state->speed * 3.6f);
    line(layout, margin, value, VALUE_POINTS, number);

    right_aligned(layout, layout->width - margin, label, LABEL_POINTS, "ALT");
    snprintf(number, sizeof number, "%.0f M", state->altitude);
    right_aligned(layout, layout->width - margin, value, VALUE_POINTS, number);

    snprintf(number, sizeof number, state->afterburner ? "THR %.0f%%  AB" : "THR %.0f%%", state->throttle * 100.0f);
    centered(layout, layout->height - margin - VALUE_POINTS * layout->scale, VALUE_POINTS, number);
}

// ammunition left and the kill count, one in each bottom corner with the throttle readout between them
static void mission_counters(const Layout *layout, const HudState *state)
{
    const float margin = MARGIN * layout->scale;
    const float y = layout->height - margin - VALUE_POINTS * layout->scale;
    char number[NUMBER_LENGTH];

    snprintf(number, sizeof number, "MSL %d/%d", state->missiles, state->pylons);
    line(layout, margin, y, VALUE_POINTS, number);

    snprintf(number, sizeof number, "TGT %d/%d", state->targets_destroyed, state->targets_total);
    right_aligned(layout, layout->width - margin, y, VALUE_POINTS, number);
}

static void distance_label(const Layout *layout, float x, float y, float range)
{
    char number[NUMBER_LENGTH];

    snprintf(number, sizeof number, "%.1f KM", range / 1000.0f);
    line(layout, x - font_width(&layout->text->font, number) * glyph_scale(layout, LABEL_POINTS) * 0.5f, y,
        LABEL_POINTS, number);
}

static void lock_box(const Layout *layout, const HudState *state)
{
    stroke(layout, state->lock_x0, state->lock_y0, state->lock_x1, state->lock_y0, LOCK_WEIGHT);
    stroke(layout, state->lock_x1, state->lock_y0, state->lock_x1, state->lock_y1, LOCK_WEIGHT);
    stroke(layout, state->lock_x1, state->lock_y1, state->lock_x0, state->lock_y1, LOCK_WEIGHT);
    stroke(layout, state->lock_x0, state->lock_y1, state->lock_x0, state->lock_y0, LOCK_WEIGHT);
    distance_label(layout, (state->lock_x0 + state->lock_x1) * 0.5f,
                   fmaxf(state->lock_y0, state->lock_y1) + LABEL_GAP * layout->scale, state->target_range);
}

// a fixed size in points, so it reads at any range
static void target_diamond(const Layout *layout, const HudState *state)
{
    const float r = DIAMOND_RADIUS * layout->scale;
    const float x = state->target_x;
    const float y = state->target_y;

    stroke(layout, x, y - r, x + r, y, DIAMOND_WEIGHT);
    stroke(layout, x + r, y, x, y + r, DIAMOND_WEIGHT);
    stroke(layout, x, y + r, x - r, y, DIAMOND_WEIGHT);
    stroke(layout, x - r, y, x, y - r, DIAMOND_WEIGHT);
    distance_label(layout, x, y + r + LABEL_GAP * layout->scale, state->target_range);
}

static void target_edge_arrow(const Layout *layout, const HudState *state)
{
    const float length = ARROW_LENGTH * layout->scale;
    const float spread = ARROW_SPREAD * layout->scale;
    const float sine = sinf(state->target_angle);
    const float cosine = cosf(state->target_angle);
    // the tip is at the marker point and the back corners trail inward
    const float local[3][2] = {{0.0f, 0.0f}, {-spread, length}, {spread, length}};
    float points[3][2];
    int i;

    for (i = 0; i < 3; i++) {
        points[i][0] = state->target_x + local[i][0] * cosine - local[i][1] * sine;
        points[i][1] = state->target_y + local[i][0] * sine + local[i][1] * cosine;
    }
    stroke(layout, points[0][0], points[0][1], points[1][0], points[1][1], ARROW_WEIGHT);
    stroke(layout, points[0][0], points[0][1], points[2][0], points[2][1], ARROW_WEIGHT);
    distance_label(layout, (points[1][0] + points[2][0]) * 0.5f, (points[1][1] + points[2][1]) * 0.5f,
                   state->target_range);
}

static void pull_up_warning(const Layout *layout)
{
    centered(layout, layout->height * PULL_UP_Y_SHARE, PULL_UP_POINTS, "PULL UP");
}

static void loadout(const Layout *layout, const HudState *state)
{
    char number[NUMBER_LENGTH];

    snprintf(number, sizeof number, "MISSILES %d/%d   TARGETS %d/%d", state->missiles, state->pylons,
             state->targets_destroyed, state->targets_total);
    centered(layout, MARGIN * layout->scale, VALUE_POINTS, number);
}

static void messages(const Layout *layout, const HudState *state)
{
    const float top = layout->height * MESSAGE_TOP;

    if (state->message == NULL) {
        // with nothing to announce, the hint is the hangar's standing line and belongs out of the way
        if (state->hint != NULL) {
            centered(layout, layout->height - (MARGIN + HINT_POINTS) * layout->scale, HINT_POINTS, state->hint);
        }
        return;
    }

    centered(layout, top, MESSAGE_POINTS, state->message);
    if (state->hint != NULL) {
        const float message_line = (float)layout->text->font.line_height * glyph_scale(layout, MESSAGE_POINTS);

        centered(layout, top + message_line, HINT_POINTS, state->hint);
    }
}

static void layers(const Layout *layout, const HudState *state)
{
    if (state->in_flight) {
        readouts(layout, state);
        marker(layout);
        mission_counters(layout, state);
        if (state->has_target) {
            if (state->locked) {
                lock_box(layout, state);
            } else if (state->target_on_screen) {
                target_diamond(layout, state);
            } else {
                target_edge_arrow(layout, state);
            }
        }
        if (state->pull_up) {
            pull_up_warning(layout);
        }
    } else {
        loadout(layout, state);
    }
    messages(layout, state);
}

void hud_draw(Text *text, const HudState *state, int width, int height)
{
    Layout layout;

    layout.text = text;
    layout.width = (float)width;
    layout.height = (float)height;
    layout.scale = (float)height / HUD_REFERENCE_HEIGHT;
    layout.offset = SHADOW_OFFSET * layout.scale;

    text_begin(text, width, height);
    // the whole layout is drawn twice: a dark copy first, so the numbers hold up over sky and over sand alike
    text_color(text, hud_shadow);
    layers(&layout, state);

    layout.offset = 0.0f;
    text_color(text, hud_color);
    layers(&layout, state);
    text_end(text);
}
