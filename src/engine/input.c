#include "engine/input.h"

#include <ctype.h>

void input_key(Input *input, unsigned char key, int down)
{
    // folded to lower case so a key released with Shift held does not stay stuck
    input->keys[tolower(key)] = (unsigned char)(down != 0);
}

void input_special(Input *input, int key, int down)
{
    if (key < 0 || key >= INPUT_SPECIAL_KEY_COUNT) {
        return;
    }
    input->special_keys[key] = (unsigned char)(down != 0);
}

void input_button(Input *input, int button, int down)
{
    if (button < 0 || button >= INPUT_BUTTON_COUNT) {
        return;
    }
    input->buttons[button] = (unsigned char)(down != 0);
}

void input_mouse_move(Input *input, int x, int y)
{
    input->mouse_x = x;
    input->mouse_y = y;
}

void input_shift(Input *input, int down)
{
    input->shift = (unsigned char)(down != 0);
}

int input_key_is_down(const Input *input, unsigned char key)
{
    return input->keys[tolower(key)];
}

int input_special_is_down(const Input *input, int key)
{
    if (key < 0 || key >= INPUT_SPECIAL_KEY_COUNT) {
        return 0;
    }
    return input->special_keys[key];
}

int input_button_is_down(const Input *input, int button)
{
    if (button < 0 || button >= INPUT_BUTTON_COUNT) {
        return 0;
    }
    return input->buttons[button];
}

int input_shift_is_down(const Input *input)
{
    return input->shift;
}
