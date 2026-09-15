#include "check.h"
#include "engine/input.h"

static void test_keys(void)
{
    Input input = {0};

    check(input_key_is_down(&input, 'w') == 0, "keys start up");

    input_key(&input, 'w', 1);
    check(input_key_is_down(&input, 'w') == 1, "key down is seen");
    input_key(&input, 'W', 0);
    check(input_key_is_down(&input, 'w') == 0, "release with Shift held still releases the key");

    input_key(&input, 'A', 1);
    check(input_key_is_down(&input, 'a') == 1, "upper case press reads as the lower case key");
    check(input_key_is_down(&input, 'A') == 1, "and can be queried either way");

    input_key(&input, 27, 1);
    check(input_key_is_down(&input, 27) == 1, "escape is tracked like any key");
}

static void test_special_keys_and_buttons(void)
{
    Input input = {0};

    input_special(&input, 101, 1);
    check(input_special_is_down(&input, 101) == 1, "special key down is seen");
    input_special(&input, 101, 0);
    check(input_special_is_down(&input, 101) == 0, "special key up is seen");
    input_special(&input, INPUT_SPECIAL_KEY_COUNT, 1);
    input_special(&input, -1, 1);
    check(input_special_is_down(&input, INPUT_SPECIAL_KEY_COUNT) == 0 && input_special_is_down(&input, -1) == 0,
          "codes outside the table are ignored");

    input_button(&input, 0, 1);
    check(input_button_is_down(&input, 0) == 1, "button down is seen");
    input_button(&input, 0, 0);
    check(input_button_is_down(&input, 0) == 0, "button up is seen");
    input_button(&input, 7, 1);
    check(input_button_is_down(&input, 7) == 0, "unknown buttons are ignored");

    input_mouse_move(&input, 40, 50);
    check(input.mouse_x == 40 && input.mouse_y == 50, "mouse position is kept");

    check(input_shift_is_down(&input) == 0, "shift starts up");
    input_shift(&input, 1);
    check(input_shift_is_down(&input) == 1, "shift down is seen");
    input_shift(&input, 0);
    check(input_shift_is_down(&input) == 0, "shift up is seen");
}

void test_input_main(void)
{
    test_keys();
    test_special_keys_and_buttons();
}
