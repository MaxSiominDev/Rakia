#ifndef TIMESTEP_H
#define TIMESTEP_H

#define TIMESTEP_HZ 120
#define TIMESTEP_DT (1.0f / (float)TIMESTEP_HZ)
#define TIMESTEP_MAX_STEPS 30

typedef struct {
    int last_ms;
    // unsimulated time in ms * TIMESTEP_HZ, so one step is 1000
    int pending;
} Timestep;

void timestep_init(Timestep *timestep, int now_ms);
// how many fixed steps to run for the time since the last call
int timestep_advance(Timestep *timestep, int now_ms);

#endif
