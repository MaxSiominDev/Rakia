#include "engine/timestep.h"

#define MAX_STALL_MS (TIMESTEP_MAX_STEPS * 1000 / TIMESTEP_HZ)

void timestep_init(Timestep *timestep, int now_ms)
{
    timestep->last_ms = now_ms;
    timestep->pending = 0;
}

int timestep_advance(Timestep *timestep, int now_ms)
{
    int elapsed = now_ms - timestep->last_ms;
    int steps;

    timestep->last_ms = now_ms;
    if (elapsed < 0) {
        elapsed = 0;
    }
    // time beyond the cap is dropped, otherwise catching up would stall the next frames too
    if (elapsed > MAX_STALL_MS) {
        elapsed = MAX_STALL_MS;
    }

    timestep->pending += elapsed * TIMESTEP_HZ;
    steps = timestep->pending / 1000;
    timestep->pending -= steps * 1000;

    return steps;
}
