#include "./includes/rsdco.h"

struct elapsed_timer* rpli_timer;
struct elapsed_timer* chkr_timer;
struct elapsed_timer* rpli_timer_core;
struct elapsed_timer* chkr_timer_core;

struct elapsed_timer* aggr_timer;
struct elapsed_timer* fail_timer;

void rsdco_init_timers() {
    rpli_timer = elapsed_timer_register();  // 0
    chkr_timer = elapsed_timer_register();  // 1

    rpli_timer_core = elapsed_timer_register();
    chkr_timer_core = elapsed_timer_register();

    aggr_timer = elapsed_timer_register();
    fail_timer = elapsed_timer_register();
}

uint64_t rsdco_get_ts_start_rpli() {
    
    uint64_t idx = __sync_fetch_and_add(&(rpli_timer->ts_idx), 1);
    get_start_ts(rpli_timer, idx);

    return idx;
}

void rsdco_get_ts_end_rpli(uint64_t idx) {
    get_end_ts(rpli_timer, idx);
}

uint64_t rsdco_get_ts_start_chkr() {
    
    uint64_t idx = __sync_fetch_and_add(&(chkr_timer->ts_idx), 1);
    get_start_ts(chkr_timer, idx);

    return idx;
}

void rsdco_get_ts_end_chkr(uint64_t idx) {
    get_end_ts(chkr_timer, idx);
}

uint64_t rsdco_get_ts_start_rpli_core() {
    
    uint64_t idx = __sync_fetch_and_add(&(rpli_timer_core->ts_idx), 1);
    get_start_ts(rpli_timer_core, idx);

    return idx;
}

void rsdco_get_ts_end_rpli_core(uint64_t idx) {
    get_end_ts(rpli_timer_core, idx);
}

uint64_t rsdco_get_ts_start_chkr_core() {
    
    uint64_t idx = __sync_fetch_and_add(&(chkr_timer_core->ts_idx), 1);
    get_start_ts(chkr_timer_core, idx);

    return idx;
}

void rsdco_get_ts_end_chkr_core(uint64_t idx) {
    get_end_ts(chkr_timer_core, idx);
}

uint64_t rsdco_get_ts_start_aggr() {
    uint64_t idx = __sync_fetch_and_add(&(aggr_timer->ts_idx), 1);
    get_start_ts(aggr_timer, idx);

    return idx;
}

void rsdco_get_ts_end_aggr(uint64_t idx) {
    get_end_ts(aggr_timer, idx);
}

uint64_t rsdco_get_ts_start_fail() {
    uint64_t idx = __sync_fetch_and_add(&(fail_timer->ts_idx), 1);
    get_start_ts(fail_timer, idx);

    return idx;
}

void rsdco_get_ts_end_fail(uint64_t idx) {
    get_end_ts(fail_timer, idx);
}

void rsdco_dump_timestamps() {
    elapsed_timer_record_manual();
}