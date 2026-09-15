#include "engine/assets.h"
#include "engine/options.h"
#include "engine/platform.h"
#include "game/game.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    static Game game;
    Options options;
    PlatformApp app;
    char error[256];

    if (options_parse(&options, argc, argv, error, sizeof error) != 0) {
        fprintf(stderr, "%s\n", error);
        options_print_usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }
    if (options.help) {
        options_print_usage(stdout, argv[0]);
        return EXIT_SUCCESS;
    }
    assets_init(options.assets_dir);

    app.context = &game;
    app.init = game_init;
    app.update = game_step;
    app.render = game_render;

    return platform_run(&options, &app, &argc, argv);
}
