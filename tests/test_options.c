#include "check.h"
#include "engine/options.h"

#include <stdarg.h>
#include <string.h>

static Options options;
static char error[128];

static int parse(int argc, ...)
{
    char *argv[8] = {"rakia"};
    va_list args;
    int i;

    va_start(args, argc);
    for (i = 0; i < argc; i++) {
        argv[i + 1] = va_arg(args, char *);
    }
    va_end(args);

    error[0] = '\0';
    return options_parse(&options, argc + 1, argv, error, sizeof error);
}

static void test_defaults(void)
{
    check(parse(0) == 0, "no arguments parse");
    check(options.windowed == 0, "fullscreen by default");
    check(options.screenshot_path == NULL, "no screenshot by default");
    check(options.bench_frames == 0, "no bench by default");
    check(options.assets_dir == NULL, "no assets override by default");
}

static void test_window(void)
{
    check(parse(2, "--window", "1280x720") == 0, "--window parses");
    check(options.windowed == 1 && options.window_width == 1280 && options.window_height == 720,
          "--window sets the size");
    check(parse(2, "--window", "1280") == -1, "--window rejects a single number");
    check(parse(2, "--window", "0x720") == -1, "--window rejects a zero width");
    check(parse(2, "--window", "640x-480") == -1, "--window rejects a negative height");
    check(parse(2, "--window", "640x480x2") == -1, "--window rejects trailing text");
    check(parse(1, "--window") == -1, "--window without a value fails");
    check(strstr(error, "--window") != NULL, "--window error names the flag");
}

static void test_screenshot_and_bench(void)
{
    check(parse(2, "--screenshot", "out.bmp") == 0, "--screenshot parses");
    check(options.screenshot_path != NULL && strcmp(options.screenshot_path, "out.bmp") == 0,
          "--screenshot keeps the path");
    check(parse(1, "--screenshot") == -1, "--screenshot without a file fails");

    check(parse(2, "--bench", "200") == 0, "--bench parses");
    check(options.bench_frames == 200, "--bench keeps the count");
    check(parse(2, "--bench", "0") == -1, "--bench rejects zero");
    check(parse(2, "--bench", "12abc") == -1, "--bench rejects trailing text");
    check(parse(2, "--bench", "99999999999") == -1, "--bench rejects an overflow");
    check(parse(1, "--bench") == -1, "--bench without a count fails");

    check(parse(4, "--screenshot", "a.bmp", "--bench", "3") == -1,
          "screenshot and bench together are rejected");
}

static void test_assets_and_unknown(void)
{
    check(parse(2, "--assets", "/tmp/assets") == 0, "--assets parses");
    check(options.assets_dir != NULL && strcmp(options.assets_dir, "/tmp/assets") == 0, "--assets keeps the dir");
    check(parse(1, "--assets") == -1, "--assets without a dir fails");

    check(parse(1, "--nope") == -1, "unknown flag fails");
    check(strstr(error, "--nope") != NULL, "unknown flag error names it");
    check(parse(1, "stray") == -1, "stray argument fails");

    check(parse(6, "--window", "800x600", "--assets", "x", "--bench", "5") == 0, "flags combine");
    check(options.window_width == 800 && options.bench_frames == 5 && strcmp(options.assets_dir, "x") == 0,
          "combined flags are all kept");
}

void test_options_main(void)
{
    test_defaults();
    test_window();
    test_screenshot_and_bench();
    test_assets_and_unknown();
}
