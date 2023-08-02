#include <vector>
#include <memory>

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <infiniband/verbs.h>

#include "./includes/connector.h"
#include "./includes/logs.h"

uint32_t rsdco_get_next_free_room(uint32_t buffer_room_idx, uint16_t buf_len) {

    buf_len += 64;
    uint32_t takes = buf_len / 64;
    uint32_t next_free_room = buffer_room_idx + takes;

    if (next_free_room > RSDCO_LOGIDX_END)
        return RSDCO_LOGIDX_START;

    return buffer_room_idx + takes;
}

void rsdco_local_writer_memory_frame_init() {

    assert(replicator_mr != nullptr);
    assert(replicator_mr != nullptr);

    struct MemoryFrame* local_log = 
        reinterpret_cast<struct MemoryFrame*>(replicator_mr->addr);

    printf("%p\n", local_log);

    local_log->room_free = RSDCO_AVAIL_ROOMS;
    local_log->next_room_free = 0;

    for (int idx = 0; idx < RSDCO_RESERVED; idx++)
        local_log->reserved_room[idx] = 0;

    std::memset(local_log->room, 0, sizeof(sizeof(uint64_t) * RSDCO_AVAIL_ROOMS));

    local_log = reinterpret_cast<struct MemoryFrame*>(checker_mr->addr);

    local_log->room_free = RSDCO_AVAIL_ROOMS;
    local_log->next_room_free = 0;

    for (int idx = 0; idx < RSDCO_RESERVED; idx++)
        local_log->reserved_room[idx] = 0;

    std::memset(local_log->room, 0, sizeof(sizeof(uint64_t) * RSDCO_AVAIL_ROOMS));
}

void replicator_propose(std::vector<struct ConnectionWriteContext>& conns, void* local_buffer, uint32_t buf_len) {

    void* local_mr_addr = replicator_mr->addr;



    for (auto& qp: conns) {

    }
}
