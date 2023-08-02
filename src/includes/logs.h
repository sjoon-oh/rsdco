#pragma once

#include <stdint.h>
#include <time.h>

#include "./connector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct __attribute__((packed)) MemoryFrame {
    uint64_t    room_free;
    uint64_t    next_room_free;
    uint32_t    reserved_room[RSDCO_RESERVED];
    uint64_t    room[RSDCO_AVAIL_ROOMS];
};

struct __attribute__((packed)) LogHeader {
    uint8_t     proc_num;
    uint8_t     message;
    uint16_t    canary;
    uint32_t    buf_len;
};

uint32_t rsdco_get_next_free_room(uint32_t, uint16_t);
void rsdco_local_writer_memory_frame_init();

#ifdef __cplusplus
}
#endif