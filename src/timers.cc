#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdint.h>
#include <pthread.h>

#include <signal.h>
#include "./includes/timers.h"

struct elapsed_timer timer_arr[20];
int32_t num_timers = 0;

void elapsed_timer_sig_handler(int signo);

void 
elapsed_timer_init(void)
{
    if (signal(SIGUSR1, elapsed_timer_sig_handler) == SIG_ERR) {
        fprintf(stderr, "[ERROR] %s: signal()", __func__);
        exit(-1);
    }
}

struct elapsed_timer *
elapsed_timer_register(void)
{
    struct elapsed_timer *timer;
    int32_t idx;

    idx = __sync_fetch_and_add(&num_timers, 1);

    if (idx == 0)
        elapsed_timer_init();

    timer = &timer_arr[idx];

    timer->start_ts = (struct timespec*)calloc(10000000, sizeof(struct timespec));
    timer->end_ts   = (struct timespec*)calloc(10000000, sizeof(struct timespec));
    timer->ts_idx   = 0;

    return timer;
}

void 
elapsed_timer_sig_handler(int signo) 
{
    if (signo == SIGUSR1) {
        char fname[64];

        for (int i = 0; i < num_timers; i++) {
            struct elapsed_timer *timer = &timer_arr[i];

            snprintf(fname, 64, "dump-%d.txt", i);
            FILE* fp = fopen(fname, "w");

            for (int64_t j = 0; j < timer->ts_idx; j++) {
                long long int time = 
                    get_elapsed_nsec(timer->start_ts[j], timer->end_ts[j]);

                fprintf(fp, "%lld\n", time);
            }

            fprintf(fp, "\n"); fflush(fp);
        }
    }
}

void 
elapsed_timer_record_manual() 
{
    char fname[64];

    for (int i = 0; i < num_timers; i++) {
        struct elapsed_timer *timer = &timer_arr[i];

        snprintf(fname, 64, "dump-%d.txt", i);
        FILE* fp = fopen(fname, "w");

        for (int64_t j = 0; j < timer->ts_idx; j++) {
            long long int time = 
                get_elapsed_nsec(timer->start_ts[j], timer->end_ts[j]);

            fprintf(fp, "%lld\n", time);
        }

        fprintf(fp, "\n"); fflush(fp);
    }
}

