#pragma once

#include <stdint.h>
#include <time.h>

#include "./rsdco_conn.h"

#define MAX_N_NODES     3

#define MSG_RELEASE     234
#define MSG_WAIT        71

#define EXCLUDE_RESET   78

#define LIVE_STATUS_ALIVE       0
#define LIVE_STATUS_DEAD        0x0a

#define HEARTBEAT_UPDATE_US     500
#define SCORE_DEFAULT   10

struct __attribute__((packed)) HeartbeatStatTable {
    uint32_t    view;
    uint32_t    exclude;
    uint32_t    msg;
    uint16_t    live_status[MAX_N_NODES];
    uint32_t    neon_sign[MAX_N_NODES];
};

struct HeartbeatLocalTable {
    uint32_t    neon_sign[MAX_N_NODES];
    int         score[MAX_N_NODES];
};

void rsdco_heartbeat_init();
void rsdco_heartbeat_release();
void rsdco_spawn_hb_tracker();