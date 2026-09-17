#!/usr/bin/env python3
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = 'assets'
HEADER_SIZE = 12
PATH_MAX = 128
ENTRY_SIZE = PATH_MAX + 8 + 8


def manifest_lines(path):
    lines = []
    with open(path) as f:
        for raw in f:
            line = raw.split('#', 1)[0].strip()
            if line:
                lines.append(line)
    return lines


def to_relative(path):
    return os.path.relpath(path, ASSETS).replace(os.sep, '/')


# a line ending in / pulls in every file under it, sorted for a reproducible pack
def resolve(line):
    if not line.endswith('/'):
        return [os.path.join(ASSETS, line)]
    base = os.path.join(ASSETS, line)
    found = [os.path.join(dirpath, name) for dirpath, _, names in os.walk(base) for name in names]
    return sorted(found, key=to_relative)


def collect(manifest_path):
    files = []
    seen = set()
    for line in manifest_lines(manifest_path):
        for path in resolve(line):
            rel = to_relative(path)
            encoded = rel.encode('utf-8')
            if len(encoded) >= PATH_MAX - 1:
                raise SystemExit(f'{rel}: relative path is {len(encoded)} bytes, must be under {PATH_MAX - 1}')
            if rel in seen:
                raise SystemExit(f'{rel}: listed twice in the manifest')
            seen.add(rel)
            files.append((rel, path))
    return files


def write_pack(files, output_path):
    index = b''
    position = 0
    for rel, path in files:
        size = os.path.getsize(path)
        index += rel.encode('utf-8').ljust(PATH_MAX, b'\0')
        index += struct.pack('<Q', HEADER_SIZE + len(files) * ENTRY_SIZE + position)
        index += struct.pack('<Q', size)
        position += size

    os.makedirs(os.path.dirname(output_path) or '.', exist_ok=True)
    with open(output_path, 'wb') as out:
        out.write(b'RKPK')
        out.write(struct.pack('<I', 1))
        out.write(struct.pack('<I', len(files)))
        out.write(index)
        for _, path in files:
            with open(path, 'rb') as f:
                out.write(f.read())

    return HEADER_SIZE + len(index) + position


def main():
    if len(sys.argv) != 3:
        raise SystemExit(f'usage: {sys.argv[0]} manifest output')
    manifest_path = os.path.abspath(sys.argv[1])
    output_path = os.path.abspath(sys.argv[2])

    os.chdir(ROOT)
    files = collect(manifest_path)
    total_size = write_pack(files, output_path)
    print(f'wrote {output_path}: {len(files)} files, {total_size} bytes')


if __name__ == '__main__':
    main()
