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
    uint32_t    hashed;
    uint16_t    buf_len;
    uint8_t     proposal;
    uint8_t     message;
};

int rsdco_slot_hash_comp(void*, void*);

uint32_t rsdco_next_free_room(uint32_t, uint16_t);

void rsdco_local_hotel_init();

void rsdco_remote_writer_hotel_init();
void rsdco_remote_receiver_hotel_init();

#ifdef __cplusplus
}
#endif