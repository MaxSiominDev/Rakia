#ifndef INPUT_H
#define INPUT_H

#define INPUT_SPECIAL_KEY_COUNT 128
#define INPUT_BUTTON_COUNT 3

typedef struct {
    unsigned char keys[256];
    unsigned char special_keys[INPUT_SPECIAL_KEY_COUNT];
    unsigned char buttons[INPUT_BUTTON_COUNT];
    // GLUT only reports modifiers inside a key callback, so this is what shift was at the last key event
    unsigned char shift;
    // window points, not pixels; see hidpi_scale
    int mouse_x;
    int mouse_y;
} Input;

void input_key(Input *input, unsigned char key, int down);
void input_special(Input *input, int key, int down);
void input_button(Input *input, int button, int down);
void input_mouse_move(Input *input, int x, int y);
void input_shift(Input *input, int down);

int input_key_is_down(const Input *input, unsigned char key);
int input_special_is_down(const Input *input, int key);
int input_button_is_down(const Input *input, int button);
int input_shift_is_down(const Input *input);

#endif
