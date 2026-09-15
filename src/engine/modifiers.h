#ifndef MODIFIERS_H
#define MODIFIERS_H

// GLUT reports the modifier keys only inside a key callback, so a lone Shift press never reaches it and the
// state is asked of the operating system instead
int modifiers_shift_is_down(void);

#endif
