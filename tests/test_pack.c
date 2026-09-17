#include "check.h"
#include "engine/pack.h"

#include <string.h>

// mirrors the write side of the pack format by hand, independently of pack.c's own reader
#define TEST_ENTRY_COUNT 2
#define TEST_HEADER_SIZE 12
#define TEST_ENTRY_SIZE (PACK_PATH_MAX + 16)

static void put_u32(unsigned char *out, unsigned int value)
{
    out[0] = (unsigned char)value;
    out[1] = (unsigned char)(value >> 8);
    out[2] = (unsigned char)(value >> 16);
    out[3] = (unsigned char)(value >> 24);
}

static void put_u64(unsigned char *out, unsigned long long value)
{
    put_u32(out, (unsigned int)value);
    put_u32(out + 4, (unsigned int)(value >> 32));
}

static void put_entry(unsigned char *out, const char *path, unsigned long long offset, unsigned long long size)
{
    memset(out, 0, PACK_PATH_MAX);
    strcpy((char *)out, path);
    put_u64(out + PACK_PATH_MAX, offset);
    put_u64(out + PACK_PATH_MAX + 8, size);
}

static size_t build_pack(unsigned char *buffer)
{
    static const char hello[] = "hello pack";
    static const char shader[] = "void main(){}";
    const size_t data_offset = TEST_HEADER_SIZE + TEST_ENTRY_COUNT * TEST_ENTRY_SIZE;

    memcpy(buffer, "RKPK", 4);
    put_u32(buffer + 4, 1);
    put_u32(buffer + 8, TEST_ENTRY_COUNT);
    put_entry(buffer + TEST_HEADER_SIZE, "hello.txt", data_offset, sizeof hello - 1);
    put_entry(buffer + TEST_HEADER_SIZE + TEST_ENTRY_SIZE, "shaders/x.vert", data_offset + sizeof hello - 1,
              sizeof shader - 1);
    memcpy(buffer + data_offset, hello, sizeof hello - 1);
    memcpy(buffer + data_offset + sizeof hello - 1, shader, sizeof shader - 1);

    return data_offset + sizeof hello - 1 + sizeof shader - 1;
}

static void test_open_and_find(void)
{
    unsigned char buffer[512];
    const size_t size = build_pack(buffer);
    Pack pack;
    const unsigned char *out;
    size_t out_size;

    check(pack_open(&pack, buffer, size) == 0, "a well-formed pack opens");
    check(pack.entry_count == TEST_ENTRY_COUNT, "the entry count matches the header");

    check(pack_find(&pack, "hello.txt", &out, &out_size) == 0, "the first entry is found");
    check(out_size == 10 && memcmp(out, "hello pack", 10) == 0, "the first entry's bytes match");

    check(pack_find(&pack, "shaders/x.vert", &out, &out_size) == 0, "the second entry is found");
    check(out_size == 13 && memcmp(out, "void main(){}", 13) == 0, "the second entry's bytes match");

    check(pack_find(&pack, "not_in_the_pack.png", &out, &out_size) == -1, "a path not in the pack is not found");
}

static void test_bad_magic(void)
{
    unsigned char buffer[512];
    Pack pack;
    const size_t size = build_pack(buffer);

    memcpy(buffer, "XXXX", 4);
    check(pack_open(&pack, buffer, size) == -1, "a wrong magic is rejected");
}

static void test_truncated_index(void)
{
    unsigned char buffer[512];
    Pack pack;

    build_pack(buffer);
    // the header still claims two entries, but the buffer given to pack_open is cut off inside the index
    check(pack_open(&pack, buffer, TEST_HEADER_SIZE + TEST_ENTRY_SIZE) == -1, "a truncated index is rejected");
}

static void test_short_buffer(void)
{
    unsigned char buffer[TEST_HEADER_SIZE - 1];
    Pack pack;

    memset(buffer, 0, sizeof buffer);
    check(pack_open(&pack, buffer, sizeof buffer) == -1, "a buffer shorter than the header is rejected");
}

// the index itself fits, but the one entry's offset and size claim bytes past the end of the buffer
static void test_out_of_bounds_entry(void)
{
    unsigned char buffer[512];
    Pack pack;
    const unsigned char *out;
    size_t out_size;
    const size_t data_offset = TEST_HEADER_SIZE + TEST_ENTRY_SIZE;

    memcpy(buffer, "RKPK", 4);
    put_u32(buffer + 4, 1);
    put_u32(buffer + 8, 1);
    put_entry(buffer + TEST_HEADER_SIZE, "overflow.bin", data_offset, sizeof buffer);

    check(pack_open(&pack, buffer, sizeof buffer) == 0, "a pack with an out-of-bounds entry still opens");
    check(pack_find(&pack, "overflow.bin", &out, &out_size) == -1,
          "an entry whose offset and size run past the end of the pack is rejected");
}

void test_pack_main(void)
{
    test_open_and_find();
    test_bad_magic();
    test_truncated_index();
    test_short_buffer();
    test_out_of_bounds_entry();
}
