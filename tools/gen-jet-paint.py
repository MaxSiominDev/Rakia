#!/usr/bin/env python3
import os
from PIL import Image, ImageDraw, ImageFilter, ImageStat

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIFFUSE = 'assets/raw/aircraft/f16_rickslash/f16_rickslash_diffuse_0.png'
OUT = 'assets/generated/f16_paint.png'
# the metal-rough map bakes the same markings as rough patches, so it is painted out too
METAL_ROUGH = 'assets/raw/aircraft/f16_rickslash/f16_rickslash_metalRough_1.jpg'
OUT_METAL_ROUGH = 'assets/generated/f16_paint_metal_rough.png'

# every region is padded past its ink, so the feather blends into clean skin
RING_MARGIN = 14
BLUR_RADIUS = 25
FEATHER_RADIUS = 8

# US-specific markings on the jet's own skin, as (x0, y0, x1, y1) rectangles on the
# 2048x2048 diffuse texture, found by cropping the texture with Pillow and confirmed on
# the jet in screenshots; the plain 902 tail number stays, IAF jets carry numbers too
REGIONS = [
    # tail fin: the AF 92 serial prefix beside 902, mirrored left and right
    (930, 995, 1050, 1185),
    (1370, 995, 1495, 1205),
    # tail fin: South Carolina lettering, mirrored left and right
    (1030, 1075, 1180, 1420),
    (1235, 1075, 1390, 1420),
    # dorsal panel: Swamp Fox script, mirrored left and right
    (325, 1575, 440, 1815),
    (423, 1570, 545, 1815),
    # star-and-bar insignia, upper wing pair
    (537, 1592, 687, 1719),
    (708, 1592, 840, 1719),
    # star-and-bar insignia, second pair on the fuselage
    (1585, 1130, 1740, 1257),
    (1300, 1475, 1420, 1563),
    # small white stars on the dorsal panel behind the canopy, mirrored left and right
    (598, 1148, 676, 1210),
    (718, 1148, 798, 1210),
]


def region_mask(size, offset, box, fill):
    x0, y0, x1, y1 = box
    mask = Image.new('L', size, 0 if fill == 255 else 255)
    ImageDraw.Draw(mask).rectangle((x0 - offset[0], y0 - offset[1], x1 - offset[0] - 1, y1 - offset[1] - 1),
                                   fill=fill)
    return mask


def paint_region(image, box):
    x0, y0, x1, y1 = box
    outer = (max(x0 - RING_MARGIN, 0), max(y0 - RING_MARGIN, 0),
             min(x1 + RING_MARGIN, image.width), min(y1 + RING_MARGIN, image.height))
    outer_crop = image.crop(outer)

    ring_mask = region_mask(outer_crop.size, outer[:2], box, fill=0)
    median = tuple(round(c) for c in ImageStat.Stat(outer_crop, mask=ring_mask).median)

    blurred = outer_crop.filter(ImageFilter.GaussianBlur(BLUR_RADIUS))
    flat = Image.new('RGB', outer_crop.size, median)
    patch = Image.blend(flat, blurred, 0.5)

    feather = region_mask(outer_crop.size, outer[:2], box, fill=255).filter(ImageFilter.GaussianBlur(FEATHER_RADIUS))
    image.paste(patch, outer[:2], feather)


def paint(image):
    for box in REGIONS:
        paint_region(image, box)
    return image


def main():
    os.chdir(ROOT)
    os.makedirs('assets/generated', exist_ok=True)

    source = Image.open(DIFFUSE).convert('RGBA')
    diffuse = paint(source.convert('RGB'))
    result = Image.merge('RGBA', (*diffuse.split(), source.getchannel('A')))
    result.save(OUT)
    print(f'wrote {OUT} at {result.size[0]}x{result.size[1]}')

    metal_rough = paint(Image.open(METAL_ROUGH).convert('RGB'))
    metal_rough.save(OUT_METAL_ROUGH)
    print(f'wrote {OUT_METAL_ROUGH} at {metal_rough.size[0]}x{metal_rough.size[1]}')


if __name__ == '__main__':
    main()
