#include <pthread.h>

#include <vector>
#include <memory>
#include <iostream>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include <unistd.h>

#include <infiniband/verbs.h>

#include "./includes/rsdco_conn.h"
#include "./includes/logs.h"
#include "./includes/hashtable.hh"

#define RSDCO_CANARY            36
#define RSDCO_MSG_COMMITTED     70

#define RSDCO_REGKEY(identifier, src, dst) \
    ((std::string(identifier) + "(" + std::string(src) + "->" + std::string(dst) + ")").c_str())

pthread_mutex_t rsdco_propose_mtx = PTHREAD_MUTEX_INITIALIZER;

extern struct ibv_mr* rsdco_rpli_mr;
extern struct ibv_mr* rsdco_chkr_mr;

extern int rsdco_sysvar_nid_int;
extern std::string rsdco_common_pd;
extern std::string sysvar_nid;
extern std::vector<std::string> rsdco_sysvar_nids;

extern std::vector<struct RsdcoConnectPair> rsdco_rpli_conn;
extern std::vector<struct RsdcoConnectPair> rsdco_chkr_conn;

const int rdsco_ht_size = (1 << 20);
const int rdsco_ht_nbuckets = (rdsco_ht_size >> 3);

RsdcoPropRequest replicator_requests;
RsdcoPropRequest checker_requests;

int rsdco_slot_hash_comp(void* a, void* b) {

    Slot* target_a = reinterpret_cast<Slot*>(a);
    Slot* target_b = reinterpret_cast<Slot*>(b);

    if (target_a->hashed == target_b->hashed) 
        return 0;

    else if (target_a->hashed > target_b->hashed)
        return 1;

    else return -1;
}

soren::hash::LfHashTable rsdco_depchecker(rdsco_ht_nbuckets, rsdco_slot_hash_comp);

void rsdco_try_insert(struct Slot* slot) {
    bool is_samekey, is_success;
    struct Slot *prev, *next;

    do {
        is_samekey = rsdco_depchecker.doSearch(slot->hashed, slot, &prev, &next);

        if (!is_samekey)
            is_success = rsdco_depchecker.doInsert(slot, prev, next);
        else
            is_success = rsdco_depchecker.doSwitch(slot, prev, next);

    } while(!is_success);
}

uint32_t rsdco_next_free_room(uint32_t buffer_room_idx, uint16_t buf_len) {

    if (buf_len % 4 != 0)
        buf_len += 4;
    
    uint32_t takes = buf_len / 4;
    uint32_t next_free_room = buffer_room_idx + takes;

    if (next_free_room > RSDCO_LOGIDX_END)
        return 0;

    return next_free_room;
}

void rsdco_local_hotel_init() {

    std::memset(&replicator_requests, 0, sizeof(struct RsdcoPropRequest));
    std::memset(&checker_requests, 0, sizeof(struct RsdcoPropRequest));

    struct MemoryHotel* hotel = 
        reinterpret_cast<struct MemoryHotel*>(rsdco_rpli_mr->addr);

    printf("replicator mr address: %p\n", hotel);

    hotel->room_free = RSDCO_AVAIL_ROOMS;
    hotel->next_room_free = 0;

    for (int idx = 0; idx < RSDCO_RESERVED; idx++)
        hotel->reserved[idx] = 0;

    std::memset(hotel->room, 0, sizeof(sizeof(uint64_t) * RSDCO_AVAIL_ROOMS));

    hotel = reinterpret_cast<struct MemoryHotel*>(rsdco_chkr_mr->addr);
    printf("checker mr address: %p\n", hotel);

    hotel->room_free = RSDCO_AVAIL_ROOMS;
    hotel->next_room_free = 0;

    for (int idx = 0; idx < RSDCO_RESERVED; idx++)
        hotel->reserved[idx] = 0;

    std::memset(hotel->room, 0, sizeof(sizeof(uint64_t) * RSDCO_AVAIL_ROOMS));
}

void rsdco_remote_writer_hotel_init() {

    struct MemoryHotel* local_hotel;
    struct MemoryHotel* remote_hotel;

    for (auto& conn: rsdco_rpli_conn) {
        
        local_hotel = reinterpret_cast<struct MemoryHotel*>(rsdco_rpli_mr->addr);
        remote_hotel = reinterpret_cast<struct MemoryHotel*>(conn.remote_mr->addr);

        local_hotel->reserved[0] = 0xdeadbeef;
        local_hotel->reserved[1] = 0;

        // Assert, 
        hartebeest_rdma_post_single_fast(
            conn.local_qp, local_hotel, remote_hotel, 128, IBV_WR_RDMA_WRITE, rsdco_rpli_mr->lkey, conn.remote_mr->rkey, 0
        );

        assert(hartebeest_rdma_poll(
            ("replicator-cq(" + sysvar_nid + "->" + std::to_string(conn.remote_node_id) + ")").c_str()
        ) == true);
    }

    for (auto& conn: rsdco_chkr_conn) {
        local_hotel = reinterpret_cast<struct MemoryHotel*>(rsdco_chkr_mr->addr);
        remote_hotel = reinterpret_cast<struct MemoryHotel*>(conn.remote_mr->addr);

        local_hotel->reserved[0] = 0xdeadbeef;
        local_hotel->reserved[1] = 0;

        // Assert, 
        hartebeest_rdma_post_single_fast(
            conn.local_qp, local_hotel, remote_hotel, 128, IBV_WR_RDMA_WRITE, rsdco_chkr_mr->lkey, conn.remote_mr->rkey, 0
        );

        assert(hartebeest_rdma_poll(
            ("checker-cq(" + sysvar_nid + "->" + std::to_string(conn.remote_node_id) + ")").c_str()
        ) == true);
    }

    sleep(3);
}

void rsdco_remote_receiver_hotel_init() {

    struct MemoryHotel* hotel;
    std::string mr_key;

    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        mr_key = RSDCO_REGKEY("replayer-mr", nid, sysvar_nid);
        hotel = reinterpret_cast<struct MemoryHotel*>(hartebeest_get_local_mr(rsdco_common_pd.c_str(), mr_key.c_str())->addr);

        assert(hotel != nullptr);

        while (hotel->reserved[0] != 0xdeadbeef)
            ;
    }

    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        mr_key = RSDCO_REGKEY("receiver-mr", nid, sysvar_nid);
        hotel = reinterpret_cast<struct MemoryHotel*>(hartebeest_get_local_mr(rsdco_common_pd.c_str(), mr_key.c_str())->addr);

        assert(hotel != nullptr);

        while (hotel->reserved[0] != 0xdeadbeef)
            ;
    }
}

void rsdco_propose(std::vector<struct RsdcoConnectPair>& conns, void* local_buffer, uint16_t buf_len) {
    
    assert(rsdco_rpli_mr != nullptr);
    assert(rsdco_chkr_mr != nullptr);

    void* local_mr_addr = rsdco_rpli_mr->addr;
    struct MemoryHotel* hotel = 
        reinterpret_cast<struct MemoryHotel*>(local_mr_addr);

    // Write Header
    uint32_t header_room = hotel->next_room_free;
    struct LogHeader* header = reinterpret_cast<struct LogHeader*>(&(hotel->room[header_room]));
    hotel->next_room_free += 1;

    header->proposal = hotel->reserved[1];
    header->buf_len = buf_len;
    header->message = 0;

    // Write payload
    uint32_t payload_room = hotel->next_room_free;
    uint32_t* payload = &(hotel->room[payload_room]);
    
    hotel->next_room_free = rsdco_next_free_room(payload_room, buf_len + sizeof(uint8_t));

    uintptr_t canary = reinterpret_cast<uintptr_t>(payload) + buf_len;
    uint8_t canary_val = RSDCO_CANARY;

    std::memcpy(payload, local_buffer, buf_len);
    std::memcpy(reinterpret_cast<void*>(canary), &canary_val, sizeof(uint8_t));

    for (auto& ctx: conns) {

        struct MemoryHotel* remote_hotel = reinterpret_cast<struct MemoryHotel*>(ctx.remote_mr->addr);
        void* remote_header = &(remote_hotel->room[header_room]);
        size_t write_sz = sizeof(struct LogHeader) + buf_len + sizeof(uint8_t);

        hartebeest_rdma_post_single_fast(
            ctx.local_qp, header, remote_header, write_sz, IBV_WR_RDMA_WRITE, rsdco_rpli_mr->lkey, ctx.remote_mr->rkey, 0
        );

        hartebeest_rdma_send_poll(ctx.local_qp);
    }

    hotel->reserved[1] += 1;
}

void rsdco_add_request_rpli(void* buf, uint16_t buf_len, void* key, uint16_t key_len, uint32_t hashed) {

    uint32_t hkey = hashed;
    if (key == nullptr) {
        ;
    }
    else
        hkey = rsdco_depchecker.doHash(key, key_len);

    RsdcoPropRequest* request_arr;
    if (hkey % rsdco_sysvar_nids.size() == 0)
        request_arr = &replicator_requests;
    else
        request_arr = &checker_requests;

    request_arr = &replicator_requests;

    // uint32_t slot_idx = atomic_fetch_add(&replicator_requests.next_free_slot, 1);
    uint32_t slot_idx = __sync_fetch_and_add(&replicator_requests.next_free_slot, 1);

    if (slot_idx < SLOT_MAX)
        ;
    else {
        while (__sync_fetch_and_add(&replicator_requests.next_free_slot, 0) != 0)
            ;
        slot_idx = __sync_fetch_and_add(&replicator_requests.next_free_slot, 1);
    }

    replicator_requests.slot[slot_idx].buf = reinterpret_cast<uint8_t*>(buf);
    replicator_requests.slot[slot_idx].buf_len = buf_len;

    replicator_requests.slot[slot_idx].hashed = hkey;
    replicator_requests.slot[slot_idx].is_ready = 1;

    rsdco_try_insert(&replicator_requests.slot[slot_idx]);

    pthread_mutex_lock(&rsdco_propose_mtx);
    rsdco_propose(
        rsdco_rpli_conn, 
        replicator_requests.slot[slot_idx].buf,
        replicator_requests.slot[slot_idx].buf_len
    );

    pthread_mutex_unlock(&rsdco_propose_mtx);
    
    if (slot_idx == (SLOT_MAX - 1)) {
        rsdco_depchecker.doResetAll();
        memset(&replicator_requests.slot, 0, sizeof(Slot) * SLOT_MAX);
        __sync_fetch_and_and(&replicator_requests.next_free_slot, 0);
    }
}

void rsdco_add_request_chkr(void* buf, uint16_t buf_len, void* key, uint16_t key_len, uint32_t hashed) {

    uint32_t hkey = hashed;

    // uint32_t slot_idx = atomic_fetch_add(&replicator_requests.next_free_slot, 1);
    uint32_t slot_idx = __sync_fetch_and_add(&checker_requests.next_free_slot, 1);

    if (slot_idx < SLOT_MAX)
        ;
    else {
        while (__sync_fetch_and_add(&checker_requests.next_free_slot, 0) != 0)
            ;
        slot_idx = __sync_fetch_and_add(&checker_requests.next_free_slot, 1);
    }

    checker_requests.slot[slot_idx].buf = reinterpret_cast<uint8_t*>(buf);
    checker_requests.slot[slot_idx].buf_len = buf_len;

    checker_requests.slot[slot_idx].hashed = hkey;
    checker_requests.slot[slot_idx].is_ready = 1;

    pthread_mutex_lock(&rsdco_propose_mtx);
    rsdco_propose(
        rsdco_chkr_conn, 
        checker_requests.slot[slot_idx].buf,
        checker_requests.slot[slot_idx].buf_len
    );

    pthread_mutex_unlock(&rsdco_propose_mtx);

    // Wait for the release
    while (checker_requests.slot[slot_idx].is_finished == 1)
        ;
    
    if (slot_idx == (SLOT_MAX - 1)) {
        memset(&checker_requests.slot, 0, sizeof(Slot) * SLOT_MAX);
        __sync_fetch_and_and(&checker_requests.next_free_slot, 0);
    }
}

void rsdco_replayer_detect(const char* detector) {

    struct ibv_mr* detector_mr = hartebeest_get_local_mr(
        rsdco_common_pd.c_str(), detector
    );

    void* local_mr_addr = detector_mr->addr;
    struct MemoryHotel* hotel = 
        reinterpret_cast<struct MemoryHotel*>(local_mr_addr);

    assert(detector_mr != nullptr);

    uint8_t local_proposal = 0;
    uint32_t header_room, payload_room;

    struct LogHeader* header;
    uint32_t* payload;

    uintptr_t canary;
    uint8_t canary_val;

    while (1) {
        
        header_room = hotel->next_room_free;
        payload_room = hotel->next_room_free + 1;

        header = reinterpret_cast<struct LogHeader*>(&(hotel->room[header_room]));
        payload = &(hotel->room[payload_room]);

        // printf("\rheader: %ld, payload: %ld, canary: %ld", header, payload, canary);

        // canary = reinterpret_cast<uint8_t*>(payload) + header->buf_len;
        canary = reinterpret_cast<uintptr_t>(payload) + header->buf_len;
        canary_val = *(reinterpret_cast<uint8_t*>(canary));

        // printf("\rRoom # %d, proposal %d, header proposal %d, len %d, canary %d, content: %s", 
        //         header_room, local_proposal, header->proposal, header->buf_len, canary_val,
        //         (char*)payload
        //     );

        if (
            (header->proposal == local_proposal) && 
                (canary_val == RSDCO_CANARY)
            ) {

            printf("\n!! Message arrival detected: %s\n", (char*)payload);
            printf("Current room # %d, proposal %d\n", header_room, header->proposal);

            hotel->next_room_free = rsdco_next_free_room(payload_room, (header->buf_len + sizeof(uint8_t)));

            local_proposal = header->proposal + 1;

            *(reinterpret_cast<uint8_t*>(canary)) = 0;

            header->buf_len = 0;
            header->proposal = 0;

            printf("-----\n");

        }
    }
}


