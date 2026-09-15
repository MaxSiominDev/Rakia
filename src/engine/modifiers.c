#include "engine/modifiers.h"

#ifdef __APPLE__

#include <objc/message.h>
#include <objc/runtime.h>

#define NS_EVENT_MODIFIER_SHIFT (1UL << 17)

typedef unsigned long (*FlagsMessage)(id, SEL);

int modifiers_shift_is_down(void)
{
    const id event_class = (id)objc_getClass("NSEvent");
    const unsigned long flags = ((FlagsMessage)objc_msgSend)(event_class, sel_registerName("modifierFlags"));

    return (flags & NS_EVENT_MODIFIER_SHIFT) != 0;
}

#elif defined(_WIN32)

#include <windows.h>

int modifiers_shift_is_down(void)
{
    return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
}

#else

int modifiers_shift_is_down(void)
{
    return 0;
}

#endif
