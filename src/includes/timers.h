#pragma once

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct elapsed_timer {
    struct timespec *start_ts;
    struct timespec *end_ts; 
    int64_t ts_idx; 
};

#define get_start_ts(timer,idx) \
    clock_gettime(CLOCK_MONOTONIC, &timer->start_ts[idx])
#define get_end_ts(timer,idx) \
    clock_gettime(CLOCK_MONOTONIC, &timer->end_ts[idx])

#define get_elapsed_nsec(t1, t2) \
    (t2.tv_nsec + t2.tv_sec * 1000000000UL - \
        t1.tv_nsec - t1.tv_sec * 1000000000UL)

void elapsed_timer_init(void);
struct elapsed_timer *elapsed_timer_register(void);

void elapsed_timer_record_manual();

#ifdef __cplusplus
}
#endif
