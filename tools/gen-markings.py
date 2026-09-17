#!/usr/bin/env python3
import math
import os
from PIL import Image, ImageDraw, ImageChops

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROUNDEL = 'assets/generated/roundel.png'
MENORAH = 'assets/generated/menorah.png'

SUPERSAMPLE = 4
BLUE = (0x00, 0x38, 0xB8)
# a paint white rather than a pure one: 210 is the brightest flat white measured on the
# jet's own diffuse texture, so a sunlit disc no longer blooms past the skin around it;
# the disc is also a flat quad facing the sun more directly than the curved skin, but that
# is a geometry difference this script cannot reach, so only the paint tone is changed
DISC_WHITE = (210, 210, 210)

# roundel: white disc, radius 100, with an outline Star of David like the flag of Israel
ROUNDEL_SIZE = 1024
DISC_R = 100.0
STAR_TIP_R = 81.0
# about a tenth of the star's point-to-point height, as on the flag
STAR_LINE_WIDTH = 0.1 * 2.0 * STAR_TIP_R


def regular_triangle(cx, cy, r, base_angle):
    return [(cx + r * math.cos(math.radians(base_angle + 120 * k)),
             cy + r * math.sin(math.radians(base_angle + 120 * k))) for k in range(3)]


# insetting a regular polygon's edges by a constant distance shrinks its circumradius by
# twice that distance (an equilateral triangle's apothem is half its circumradius), so a
# band of constant width is the ring between two triangles at r + width and r - width
def triangle_band(size, cx, cy, base_angle, outer_r, inner_r):
    outer = Image.new('L', size, 0)
    ImageDraw.Draw(outer).polygon(regular_triangle(cx, cy, outer_r, base_angle), fill=255)
    inner = Image.new('L', size, 0)
    ImageDraw.Draw(inner).polygon(regular_triangle(cx, cy, inner_r, base_angle), fill=255)
    return ImageChops.subtract(outer, inner)


def draw_roundel():
    size = (ROUNDEL_SIZE * SUPERSAMPLE, ROUNDEL_SIZE * SUPERSAMPLE)
    cx = cy = size[0] / 2.0
    scale = size[0] / (2.0 * DISC_R)

    disc = Image.new('L', size, 0)
    r = DISC_R * scale
    ImageDraw.Draw(disc).ellipse([cx - r, cy - r, cx + r, cy + r], fill=255)

    outer_r = STAR_TIP_R * scale
    inner_r = (STAR_TIP_R - 2.0 * STAR_LINE_WIDTH) * scale
    up = triangle_band(size, cx, cy, -90, outer_r, inner_r)
    down = triangle_band(size, cx, cy, 90, outer_r, inner_r)
    star = ImageChops.lighter(up, down)

    # DISC_WHITE under the star and the disc's own edge, so downsampling never blends in black
    image = Image.new('RGBA', size, DISC_WHITE + (0,))
    image.paste(DISC_WHITE + (255,), mask=disc)
    image.paste(BLUE + (255,), mask=star)
    return image.resize((ROUNDEL_SIZE, ROUNDEL_SIZE), Image.LANCZOS)


# menorah: a solid silhouette, three nested pairs of rounded arms rising to a common
# height with small cups, a straight stem and a stepped base; no olive branches, no text
CUP_R = 32.0
STEM_WIDTH = 44.0
ARM_WIDTH = 38.0
MARGIN = 24.0
# how far out each pair's cups sit, inner pair first; also its ellipse's vertical radius,
# so each arm leaves the stem sideways and turns upright into its cup
ARM_REACH = [130.0, 250.0, 380.0]
STEM_BELOW_ARMS = 40.0
# two tiers, each (half width, height), top tier first
BASE_TIERS = [(80.0, 70.0), (150.0, 90.0)]


def draw_menorah():
    top_y = MARGIN + CUP_R
    attach_y = [top_y + reach for reach in ARM_REACH]
    stem_bottom_y = attach_y[-1] + STEM_BELOW_ARMS
    base_bottom_y = stem_bottom_y + sum(height for _, height in BASE_TIERS)
    half_width = ARM_REACH[-1] + CUP_R + MARGIN

    width = round(2.0 * half_width)
    height = round(base_bottom_y + MARGIN)
    cx = width / 2.0
    s = float(SUPERSAMPLE)
    size = (width * SUPERSAMPLE, height * SUPERSAMPLE)

    coverage = Image.new('L', size, 0)
    draw = ImageDraw.Draw(coverage)
    draw.line([(cx * s, top_y * s), (cx * s, stem_bottom_y * s)], fill=255, width=round(STEM_WIDTH * s))

    for reach, att in zip(ARM_REACH, attach_y):
        # one ellipse per pair, centered on the stem at the top row; its lower half runs
        # from the right cup through the stem attachment point to the left cup
        bbox = [(cx - reach) * s, (2.0 * top_y - att) * s, (cx + reach) * s, att * s]
        draw.arc(bbox, 0, 180, fill=255, width=round(ARM_WIDTH * s))

    tier_y = stem_bottom_y
    for tier_half_width, tier_height in BASE_TIERS:
        draw.rectangle([(cx - tier_half_width) * s, tier_y * s,
                        (cx + tier_half_width) * s, (tier_y + tier_height) * s], fill=255)
        tier_y += tier_height

    cups = [(cx, top_y)] + [(cx + side * reach, top_y) for reach in ARM_REACH for side in (1, -1)]
    for bx, by in cups:
        draw.ellipse([(bx - CUP_R) * s, (by - CUP_R) * s, (bx + CUP_R) * s, (by + CUP_R) * s], fill=255)

    # blue under the whole canvas, so downsampling blends alpha without shifting color
    image = Image.new('RGBA', size, BLUE + (0,))
    image.paste(BLUE + (255,), mask=coverage)
    return image.resize((width, height), Image.LANCZOS)


def main():
    os.chdir(ROOT)
    os.makedirs('assets/generated', exist_ok=True)
    roundel = draw_roundel()
    roundel.save(ROUNDEL)
    menorah = draw_menorah()
    menorah.save(MENORAH)
    print(f'wrote {ROUNDEL} at {roundel.size[0]}x{roundel.size[1]}')
    print(f'wrote {MENORAH} at {menorah.size[0]}x{menorah.size[1]}')


if __name__ == '__main__':
    main()
