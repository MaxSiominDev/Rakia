#include "check.h"
#include "engine/post.h"

static void test_sample_count(void)
{
    check(post_sample_count(4, 8) == 4, "the requested count stands when the driver allows more");
    check(post_sample_count(4, 4) == 4, "and when it allows exactly that many");
    check(post_sample_count(4, 2) == 2, "a driver that allows fewer clamps the request down");
    check(post_sample_count(4, 0) == 0, "a driver that reports no multisampling at all disables it");
}

static void test_bloom_dimension(void)
{
    check(post_bloom_dimension(1280, 2) == 640, "half of an even size divides evenly");
    check(post_bloom_dimension(1280, 4) == 320, "quarter likewise");
    check(post_bloom_dimension(256, 2) == 128, "the CI smoke render halves cleanly");
    check(post_bloom_dimension(256, 4) == 64, "and quarters cleanly");
    check(post_bloom_dimension(1281, 2) == 640, "an odd size rounds down");
    check(post_bloom_dimension(1, 4) == 1, "a target too small to divide still gets at least one texel");
    check(post_bloom_dimension(3, 4) == 1, "so does one that would otherwise round to zero");
}

void test_post_main(void)
{
    test_sample_count();
    test_bloom_dimension();
}
