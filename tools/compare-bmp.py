#!/usr/bin/env python3
import argparse
import struct
import sys

OUTLIER_DISTANCE = 32


def read_bmp(path):
    with open(path, 'rb') as f:
        data = f.read()
    if data[:2] != b'BM':
        raise SystemExit(f'{path}: not a BMP file')
    offset, width, height, planes, depth = struct.unpack_from('<I4xiihh', data, 10)
    if depth != 24 or planes != 1 or height <= 0:
        raise SystemExit(f'{path}: expected an uncompressed bottom-up 24-bit BMP')
    stride = (width * 3 + 3) & ~3
    rows = [data[offset + y * stride:offset + y * stride + width * 3] for y in range(height)]
    return width, height, b''.join(rows)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('reference')
    parser.add_argument('candidate')
    parser.add_argument('--mean', type=float, default=2.0,
                        help='largest allowed mean absolute channel difference, 0..255')
    parser.add_argument('--outliers', type=float, default=0.005,
                        help='largest allowed fraction of pixels off by more than %d' % OUTLIER_DISTANCE)
    args = parser.parse_args()

    ref_w, ref_h, ref = read_bmp(args.reference)
    cand_w, cand_h, cand = read_bmp(args.candidate)
    if (ref_w, ref_h) != (cand_w, cand_h):
        raise SystemExit(f'size mismatch: reference {ref_w}x{ref_h}, candidate {cand_w}x{cand_h}')

    total = 0
    outliers = 0
    for i in range(0, len(ref), 3):
        distance = max(abs(ref[i] - cand[i]), abs(ref[i + 1] - cand[i + 1]), abs(ref[i + 2] - cand[i + 2]))
        total += abs(ref[i] - cand[i]) + abs(ref[i + 1] - cand[i + 1]) + abs(ref[i + 2] - cand[i + 2])
        if distance > OUTLIER_DISTANCE:
            outliers += 1

    pixels = ref_w * ref_h
    mean = total / (pixels * 3)
    fraction = outliers / pixels
    print(f'mean difference {mean:.2f}, {fraction * 100:.2f}% of pixels off by more than {OUTLIER_DISTANCE}')

    if mean > args.mean or fraction > args.outliers:
        print(f'exceeds the allowed mean {args.mean} or outlier fraction {args.outliers}')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
