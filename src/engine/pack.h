#ifndef PACK_H
#define PACK_H

#include <stddef.h>

#define PACK_PATH_MAX 128

typedef struct {
    const unsigned char *data;
    size_t size;
    size_t entry_count;
} Pack;

// validates the magic, version and that the index fits within size; 0 on success, -1 on a bad magic,
// a bad version, or a truncated index
int pack_open(Pack *pack, const unsigned char *data, size_t size);
// looks up a path in an already-open pack; 0 on success with *out/*out_size pointing inside pack->data,
// -1 if the path is not present or the entry's offset/size do not fit inside the pack
int pack_find(const Pack *pack, const char *relative, const unsigned char **out, size_t *out_size);

#endif
