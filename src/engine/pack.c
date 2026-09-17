#include "engine/pack.h"

#include <stdint.h>
#include <string.h>

#define PACK_HEADER_SIZE 12
#define PACK_ENTRY_SIZE (PACK_PATH_MAX + 8 + 8)

static uint32_t read_u32(const unsigned char *bytes)
{
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

static uint64_t read_u64(const unsigned char *bytes)
{
    return (uint64_t)read_u32(bytes) | (uint64_t)read_u32(bytes + 4) << 32;
}

int pack_open(Pack *pack, const unsigned char *data, size_t size)
{
    size_t count;

    if (data == NULL || size < PACK_HEADER_SIZE || memcmp(data, "RKPK", 4) != 0 || read_u32(data + 4) != 1) {
        return -1;
    }

    // dividing the available space instead of multiplying count by the entry size sidesteps overflow
    count = read_u32(data + 8);
    if (count > (size - PACK_HEADER_SIZE) / PACK_ENTRY_SIZE) {
        return -1;
    }

    pack->data = data;
    pack->size = size;
    pack->entry_count = count;

    return 0;
}

int pack_find(const Pack *pack, const char *relative, const unsigned char **out, size_t *out_size)
{
    size_t i;

    for (i = 0; i < pack->entry_count; i++) {
        const unsigned char *entry = pack->data + PACK_HEADER_SIZE + i * PACK_ENTRY_SIZE;
        uint64_t offset;
        uint64_t file_size;

        if (strncmp((const char *)entry, relative, PACK_PATH_MAX) != 0) {
            continue;
        }
        offset = read_u64(entry + PACK_PATH_MAX);
        file_size = read_u64(entry + PACK_PATH_MAX + 8);
        if (offset > (uint64_t)pack->size || file_size > (uint64_t)pack->size - offset) {
            return -1;
        }
        *out = pack->data + offset;
        *out_size = (size_t)file_size;

        return 0;
    }

    return -1;
}
