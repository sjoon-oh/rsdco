#pragma once

#include "./rsdco_conn.h"
#include "./rsdco_heartbeat.h"
#include "./rsdco_hotel.h"
#include "./rsdco_reqs.h"
#include "./rsdco_wrkr.h"
#include "./rsdco_rule.h"

#include "./timers.h"

#ifdef __cplusplus
extern "C" {
#endif

void rsdco_init_timers();

uint64_t rsdco_get_ts_start_rpli();
void rsdco_get_ts_end_rpli(uint64_t);

uint64_t rsdco_get_ts_start_chkr();
void rsdco_get_ts_end_chkr(uint64_t);

uint64_t rsdco_get_ts_start_rpli_core();
void rsdco_get_ts_end_rpli_core(uint64_t);

uint64_t rsdco_get_ts_start_chkr_core();
void rsdco_get_ts_end_chkr_core(uint64_t);

uint64_t rsdco_get_ts_start_aggr();
void rsdco_get_ts_end_aggr(uint64_t);

uint64_t rsdco_get_ts_start_fail();
void rsdco_get_ts_end_fail(uint64_t);


void rsdco_dump_timestamps();

#ifdef __cplusplus
}
#endif