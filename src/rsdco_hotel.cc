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
#include "./includes/rsdco_hotel.h"
#include "./includes/rsdco_reqs.h"
#include "./includes/hashtable.hh"



#define RSDCO_REGKEY(identifier, src, dst) \
    ((std::string(identifier) + "(" + std::string(src) + "->" + std::string(dst) + ")").c_str())

extern struct ibv_mr* rsdco_rpli_mr;
extern struct ibv_mr* rsdco_chkr_mr;


extern int rsdco_sysvar_nid_int;
extern std::string rsdco_common_pd;
extern std::string sysvar_nid;
extern std::vector<std::string> rsdco_sysvar_nids;


extern RsdcoPropRequest rpli_reqs;
extern RsdcoPropRequest chkr_reqs;

extern std::vector<struct RsdcoConnectPair> rsdco_rpli_conn;
extern std::vector<struct RsdcoConnectPair> rsdco_chkr_conn;

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

    std::memset(&rpli_reqs, 0, sizeof(struct RsdcoPropRequest));
    std::memset(&chkr_reqs, 0, sizeof(struct RsdcoPropRequest));

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

    sleep(1);
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
        printf("%s for receive ready\n", mr_key.c_str());
    }

    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        mr_key = RSDCO_REGKEY("receiver-mr", nid, sysvar_nid);
        hotel = reinterpret_cast<struct MemoryHotel*>(hartebeest_get_local_mr(rsdco_common_pd.c_str(), mr_key.c_str())->addr);

        assert(hotel != nullptr);

        while (hotel->reserved[0] != 0xdeadbeef)
            ;
        printf("%s for receive ready\n", mr_key.c_str());
    }
}

