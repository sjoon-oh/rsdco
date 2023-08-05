#include "./includes/rsdco.h"

struct elapsed_timer* rpli_timer;
struct elapsed_timer* chkr_timer;

void rsdco_init_timers() {
    rpli_timer = elapsed_timer_register();  // 0
    chkr_timer = elapsed_timer_register();  // 1
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







void rsdco_dump_timestamps() {
    elapsed_timer_record_manual();
}