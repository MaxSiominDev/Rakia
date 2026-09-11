#include "check.h"
#include "engine/timestep.h"

static void test_steps_follow_elapsed_time(void)
{
    Timestep t;

    timestep_init(&t, 1000);
    check(timestep_advance(&t, 1000) == 0, "no time, no step");
    check(timestep_advance(&t, 1008) == 0, "8 ms is short of one 120 Hz step");
    check(timestep_advance(&t, 1017) == 2, "17 ms in total gives two steps");
    check(timestep_advance(&t, 1017) == 0, "the same time again gives nothing");
    check(timestep_advance(&t, 1025) == 1, "the remainder carries over");
}

static void test_no_drift_over_a_second(void)
{
    Timestep t;
    int total = 0;
    int now;

    timestep_init(&t, 0);
    for (now = 10; now <= 1000; now += 10) {
        total += timestep_advance(&t, now);
    }
    check(total == 120, "one second of 10 ms frames runs exactly 120 steps");

    timestep_init(&t, 0);
    total = 0;
    for (now = 7; now <= 1001; now += 7) {
        total += timestep_advance(&t, now);
    }
    check(total == 120, "1001 ms of 7 ms frames runs 120 steps");
}

static void test_stall_is_capped(void)
{
    Timestep t;

    timestep_init(&t, 0);
    check(timestep_advance(&t, 5000) == TIMESTEP_MAX_STEPS, "a long stall runs the capped step count");
    check(timestep_advance(&t, 5008) == 0, "nothing of the stall is carried over");
    check(timestep_advance(&t, 5017) == 2, "normal frames resume after the stall");
}

static void test_clock_going_backwards(void)
{
    Timestep t;

    timestep_init(&t, 500);
    check(timestep_advance(&t, 400) == 0, "an earlier time gives no steps");
    check(timestep_advance(&t, 417) == 2, "and the clock resyncs to the earlier time");
}

void test_timestep_main(void)
{
    test_steps_follow_elapsed_time();
    test_no_drift_over_a_second();
    test_stall_is_capped();
    test_clock_going_backwards();
}
