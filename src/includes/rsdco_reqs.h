#pragma once

#include <stdint.h>
#include <time.h>

#include <infiniband/verbs.h>

#include "./rsdco_hotel.h"


#ifdef __cplusplus
extern "C" {
#endif

struct Slot {
    struct Slot*    next_slot;
    uint32_t        hashed;

    uint8_t         is_ready;
    uint8_t         is_blocked;

    uint8_t*        buf;
    uint16_t        buf_len;

    uint8_t*        key;
    uint16_t        key_len;
};

#define SLOT_MAX    512

struct RsdcoPropRequest {
    struct Slot     slot[SLOT_MAX];
    uint32_t        next_free_slot;
    uint32_t        procd;
};

void rsdco_add_request(void*, uint16_t, void*, uint16_t, uint32_t, int (*)(uint32_t));
void rsdco_request_to_rpli(void*, uint16_t, void*, uint16_t, uint32_t, uint8_t);
void rsdco_request_to_chkr(void*, uint16_t, void*, uint16_t, uint32_t, uint8_t);

void rsdco_detect_poll(const char*, struct ibv_mr*, void (*)(struct MemoryHotel*, uint32_t, uint32_t, void (*)(void*, uint16_t)), void (*)(void*, uint16_t));

void rsdco_detect_action_rply(struct MemoryHotel*, uint32_t, uint32_t, void (*)(void*, uint16_t));
void rsdco_detect_action_rcvr(struct MemoryHotel*, uint32_t, uint32_t, void (*)(void*, uint16_t));

uint32_t rsdco_hash(const char*, int);

#ifdef __cplusplus
}
#endif