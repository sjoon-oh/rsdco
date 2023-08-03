#pragma once

#include <stdint.h>
#include <time.h>

#include "./rsdco_conn.h"

#ifdef __cplusplus
extern "C" {
#endif

struct __attribute__((packed)) MemoryHotel {
    uint32_t    room_free;
    uint32_t    next_room_free;                 
    uint32_t    reserved[RSDCO_RESERVED];   // 128, 2 Rooms reserved
    uint32_t    room[RSDCO_AVAIL_ROOMS];
};

struct __attribute__((packed)) LogHeader {
    uint8_t     proposal;
    uint8_t     message;
    uint16_t    buf_len;
};

struct Slot {
    struct Slot*    next_slot;
    uint32_t        hashed;

    uint8_t         is_ready;
    uint8_t         is_finished;

    uint8_t*        buf;
    uint16_t        buf_len;
};

#define SLOT_MAX    128
struct RsdcoPropRequest {
    struct Slot     slot[SLOT_MAX];

    uint32_t        next_free_slot;
};

int rsdco_slot_hash_comp(void*, void*);

uint32_t rsdco_next_free_room(uint32_t, uint16_t);
void rsdco_local_hotel_init();
void rsdco_remote_writer_hotel_init();
void rsdco_remote_receiver_hotel_init();

void rsdco_add_request_rpli(void*, uint16_t, void*, uint16_t, uint32_t);
void rsdco_replayer_detect(const char*);

#ifdef __cplusplus
}
#endif