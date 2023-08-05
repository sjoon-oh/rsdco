#pragma once

#include "./rsdco_conn.h"
#include "./rsdco_hotel.h"
#include "./rsdco_reqs.h"
#include "./rsdco_wrkr.h"
#include "./rsdco_rule.h"

#include "./timers.h"

void rsdco_init_timers();

uint64_t rsdco_get_ts_start_rpli();
void rsdco_get_ts_end_rpli(uint64_t);

uint64_t rsdco_get_ts_start_chkr();
void rsdco_get_ts_end_chkr(uint64_t);