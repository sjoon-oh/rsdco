#include <vector>
#include <string>
#include <memory>

#include <cstring>

#include <assert.h>
#include <stdlib.h>


#include "../hartebeest/src/includes/hartebeest-c.h"

#include "./includes/hashtable.hh"
#include "./includes/rsdco_reqs.h"

#include "./includes/rsdco.h"

const int rdsco_ht_size = (1 << 20);
const int rdsco_ht_nbuckets = (rdsco_ht_size >> 3);

extern int rsdco_sysvar_nid_int;

extern struct ibv_mr* rsdco_rpli_mr;
extern struct ibv_mr* rsdco_chkr_mr;

#define RSDCO_CANARY            36
#define RSDCO_MSG_COMMITTED     70

#define RSDCO_MSG_ACK           123
#define RSDCO_MSG_PURE          12
#define RSDCO_MSG_QUESTION      67
#define RSDCO_HDR_ONLY          90

#define RSDCO_REGKEY(identifier, src, dst) \
    ((std::string(identifier) + "(" + std::string(src) + "->" + std::string(dst) + ")").c_str())

RsdcoPropRequest rpli_reqs;
RsdcoPropRequest chkr_reqs;

pthread_mutex_t rsdco_propose_rpli_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rsdco_propose_chkr_mtx = PTHREAD_MUTEX_INITIALIZER;

extern std::vector<struct RsdcoConnectPair> rsdco_rpli_conn;
extern std::vector<struct RsdcoConnectPair> rsdco_chkr_conn;

extern std::string rsdco_common_pd;
extern std::string sysvar_nid;
extern std::vector<std::string> rsdco_sysvar_nids;

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

uint32_t rsdco_hash(const char* buf, int buf_len) {
    return rsdco_depchecker.doHash(buf, buf_len);
}

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


void rsdco_rdma_write_rpli(void* local_buffer, uint16_t buf_len, uint32_t hashed, uint8_t msg) {
    
    assert(rsdco_rpli_mr != nullptr);

    struct MemoryHotel* hotel = 
        reinterpret_cast<struct MemoryHotel*>(rsdco_rpli_mr->addr);

    // Write Header
    uint32_t header_room = hotel->next_room_free;
    struct LogHeader* header = reinterpret_cast<struct LogHeader*>(&(hotel->room[header_room]));
    hotel->next_room_free += 2;

    header->proposal = hotel->reserved[1];
    header->buf_len = buf_len;
    header->message = msg;
    header->hashed = hashed;

    // Write payload
    uint32_t payload_room = hotel->next_room_free;
    uint32_t* payload = &(hotel->room[payload_room]);
    
    hotel->next_room_free = rsdco_next_free_room(payload_room, buf_len + sizeof(uint8_t));

    uintptr_t canary = reinterpret_cast<uintptr_t>(payload) + buf_len;
    uint8_t canary_val = RSDCO_CANARY;

    std::memcpy(payload, local_buffer, buf_len);
    std::memcpy(reinterpret_cast<void*>(canary), &canary_val, sizeof(uint8_t));

    for (auto& ctx: rsdco_rpli_conn) {

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

void rsdco_rdma_write_chkr(void* local_buffer, uint16_t buf_len, uint32_t hashed, int owned) {
    
    assert(rsdco_chkr_mr != nullptr);

    struct MemoryHotel* hotel = 
        reinterpret_cast<struct MemoryHotel*>(rsdco_chkr_mr->addr);

    // Write Header
    uint32_t header_room = hotel->next_room_free;
    struct LogHeader* header = reinterpret_cast<struct LogHeader*>(&(hotel->room[header_room]));
    hotel->next_room_free += 2;

    header->proposal = hotel->reserved[1];
    header->buf_len = buf_len;
    header->hashed = hashed;

    // Write payload
    uint32_t payload_room = hotel->next_room_free;
    uint32_t* payload = &(hotel->room[payload_room]);
    
    hotel->next_room_free = rsdco_next_free_room(payload_room, buf_len + sizeof(uint8_t));

    uintptr_t canary = reinterpret_cast<uintptr_t>(payload) + buf_len;
    uint8_t canary_val = RSDCO_CANARY;

    std::memcpy(payload, local_buffer, buf_len);
    std::memcpy(reinterpret_cast<void*>(canary), &canary_val, sizeof(uint8_t));

    struct MemoryHotel* remote_hotel;
    void* remote_header;
    size_t write_sz;

    for (auto& ctx: rsdco_chkr_conn) {

        remote_hotel = reinterpret_cast<struct MemoryHotel*>(ctx.remote_mr->addr);
        remote_header = &(remote_hotel->room[header_room]);
        
        if (ctx.remote_node_id == owned) {
            write_sz = sizeof(struct LogHeader) + buf_len + sizeof(uint8_t);
            header->message = RSDCO_MSG_QUESTION;
        }
        else {
            write_sz = sizeof(struct LogHeader);
            header->message = RSDCO_HDR_ONLY;
        }

        hartebeest_rdma_post_single_fast(
            ctx.local_qp, header, remote_header, write_sz, IBV_WR_RDMA_WRITE, rsdco_chkr_mr->lkey, ctx.remote_mr->rkey, 0
        );

        hartebeest_rdma_send_poll(ctx.local_qp);
    }

    hotel->reserved[1] += 1;
}


void rsdco_request_to_rpli(void* buf, uint16_t buf_len, void* key, uint16_t key_len, uint32_t hashed, uint8_t msg) {

    uint32_t slot_idx;
    while ( !((slot_idx = __sync_fetch_and_add(&rpli_reqs.next_free_slot, 1)) < SLOT_MAX) )
        ;

    // Metadata for unlock
    rpli_reqs.slot[slot_idx].buf = reinterpret_cast<uint8_t*>(buf);
    rpli_reqs.slot[slot_idx].buf_len = buf_len;

    rpli_reqs.slot[slot_idx].key = reinterpret_cast<uint8_t*>(key);
    rpli_reqs.slot[slot_idx].key_len = key_len;

    rpli_reqs.slot[slot_idx].hashed = hashed;
    rpli_reqs.slot[slot_idx].is_ready = 1;
    rpli_reqs.slot[slot_idx].is_blocked = 1;

    rsdco_try_insert(&rpli_reqs.slot[slot_idx]);
    pthread_mutex_lock(&rsdco_propose_rpli_mtx);

    // Search for next.
    struct List* bucket = rsdco_depchecker.getBucket(hashed);
    struct Slot* cur_slot = &rpli_reqs.slot[slot_idx];
    struct Slot* next_slot;

    if (rpli_reqs.slot[slot_idx].is_ready == 1) {

        if (IS_MARKED_AS_DELETED(rpli_reqs.slot[slot_idx].next_slot)) {
            // printf("Marked detected, searching for latest link... : %ld\n", hashed);

            while (cur_slot != &bucket->head) {

                next_slot = cur_slot->next_slot;
                if (IS_MARKED_AS_DELETED(next_slot)) {
                    cur_slot->is_ready = 0;
                    cur_slot = GET_UNMARKED_REFERENCE(next_slot);
                }
                else {
                    if (cur_slot->hashed == hashed) break;
                    else 
                        assert(false);
                }
            }
        }

        uint64_t idx = rsdco_get_ts_start_rpli_core();
        rsdco_rdma_write_rpli(cur_slot->buf, cur_slot->buf_len, hashed, msg);
        rsdco_get_ts_end_rpli_core(idx);

        cur_slot->is_ready = 0;
        rpli_reqs.slot[slot_idx].is_ready = 0;
    }
    // else
    //     printf("Not ready: %ld, skipped, idx: %ld\n", hashed, slot_idx);

    rpli_reqs.procd += 1;
    if (rpli_reqs.procd == (SLOT_MAX)) {
        rpli_reqs.procd = 0;
        rsdco_depchecker.doResetAll();
        // memset(&rpli_reqs, 0, sizeof(struct RsdcoPropRequest));

        memset(rpli_reqs.slot, 0, sizeof(struct Slot) * SLOT_MAX);
        __sync_fetch_and_and(&rpli_reqs.next_free_slot, 0);
    }

    pthread_mutex_unlock(&rsdco_propose_rpli_mtx);
}

void rsdco_request_to_chkr(void* buf, uint16_t buf_len, void* key, uint16_t key_len, uint32_t hashed, int owned) {
    pthread_mutex_lock(&rsdco_propose_chkr_mtx);

    uint32_t slot_idx = chkr_reqs.next_free_slot;
    // printf("Ask for %ld to %ld, slot idx %ld\n", hashed, owned, slot_idx);

    if (slot_idx == SLOT_MAX) {
        slot_idx = chkr_reqs.next_free_slot = 0;
        memset(&chkr_reqs, 0, sizeof(struct RsdcoPropRequest));
    }

    chkr_reqs.slot[slot_idx].buf = reinterpret_cast<uint8_t*>(buf);
    chkr_reqs.slot[slot_idx].buf_len = buf_len;

    chkr_reqs.slot[slot_idx].key = reinterpret_cast<uint8_t*>(key);
    chkr_reqs.slot[slot_idx].key_len = key_len;

    chkr_reqs.slot[slot_idx].hashed = hashed;
    chkr_reqs.slot[slot_idx].is_ready = 1;

    __sync_fetch_and_add(&chkr_reqs.slot[slot_idx].is_blocked, 1);
    
    uint64_t idx = rsdco_get_ts_start_chkr_core();
    rsdco_rdma_write_chkr(
        chkr_reqs.slot[slot_idx].buf,
        chkr_reqs.slot[slot_idx].buf_len,
        hashed,
        owned
    );
    
    rsdco_get_ts_end_chkr_core(idx);

    while (__sync_fetch_and_add(&chkr_reqs.slot[slot_idx].is_blocked, 0) == 1)
        ;
    
    // printf("Ask for %ld unlocked\n", hashed, owned, slot_idx);
    
    chkr_reqs.next_free_slot += 1;
    pthread_mutex_unlock(&rsdco_propose_chkr_mtx);
}


void rsdco_add_request(void* buf, uint16_t buf_len, void* key, uint16_t key_len, uint32_t hashed, int (*ruler)(uint32_t)) {
    
    uint32_t hkey = hashed;
    if (key != nullptr)
        hkey = rsdco_depchecker.doHash(key, key_len);

    uint64_t ts_idx;
    int owned = ruler(hkey);

    // printf("Add request: %ld, owned by %ld\n", hkey, owned);

    if (owned == rsdco_sysvar_nid_int) {
        ts_idx = rsdco_get_ts_start_rpli();
        rsdco_request_to_rpli(buf, buf_len, key, key_len, hkey, RSDCO_MSG_PURE);
        rsdco_get_ts_end_rpli(ts_idx);
    }
    else {
        ts_idx = rsdco_get_ts_start_chkr();
        rsdco_request_to_chkr(buf, buf_len, key, key_len, hkey, owned);
        rsdco_get_ts_end_chkr(ts_idx);
    }
}

void rsdco_detect_poll(
        const char* detector, struct ibv_mr* detector_mr,
        void (*detect_func)(struct MemoryHotel*, uint32_t, uint32_t, void (*)(void*, uint16_t)), 
        void (*user_action)(void*, uint16_t)
    ) {

    assert(detector_mr != nullptr);

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

    uint32_t prev_header_room;

    while (1) {
        
        header_room = hotel->next_room_free;
        payload_room = hotel->next_room_free + 2;

        header = reinterpret_cast<struct LogHeader*>(&(hotel->room[header_room]));
        payload = &(hotel->room[payload_room]);

        canary = reinterpret_cast<uintptr_t>(payload) + header->buf_len;
        canary_val = *(reinterpret_cast<uint8_t*>(canary));

        if (
            ((header->proposal == local_proposal) && (canary_val == RSDCO_CANARY)) ||
            ((header->proposal == local_proposal) && (header->message == RSDCO_HDR_ONLY)) 
            ) {
            
            // float perc = header_room / RSDCO_LOGIDX_END * 100;
            // if ((header->message == RSDCO_HDR_ONLY))
            //     printf("%s HEADER !! detected(%ld): %ld\n", detector, header_room, header->hashed);
            // else
            //     printf("%s !! detected(%ld): %ld\n", detector, header_room, header->hashed);

            hotel->next_room_free = rsdco_next_free_room(payload_room, (header->buf_len + sizeof(uint8_t)));

            local_proposal = header->proposal + 1;
            
            *(reinterpret_cast<uint8_t*>(canary)) = 0;

            detect_func(hotel, header_room, prev_header_room, user_action);
            prev_header_room = header_room;
        }
    }
}

// 
void rsdco_detect_action_rply(struct MemoryHotel* hotel, uint32_t header_room, uint32_t prev_header_room, void (*user_action)(void*, uint16_t)) {

    // For replay, previous header room are passed here.
    struct LogHeader* header = reinterpret_cast<struct LogHeader*>(&(hotel->room[header_room]));
    uint32_t* payload = &(hotel->room[header_room + 2]);

    struct LogHeader* prev_header = reinterpret_cast<struct LogHeader*>(&(hotel->room[prev_header_room]));
    uint32_t* prev_payload = &(hotel->room[prev_header_room + 2]);

    // Case when unlock needed, no need 
    if (header->message == RSDCO_MSG_ACK) {
        
        // printf("ACK received: %ld, curr pending chkr: %ld, slotidx: %ld\n", header->hashed, chkr_reqs.slot[chkr_reqs.next_free_slot].hashed, chkr_reqs.next_free_slot);
        if (chkr_reqs.slot[chkr_reqs.next_free_slot].hashed == header->hashed) {
            __sync_fetch_and_and(&(chkr_reqs.slot[chkr_reqs.next_free_slot].is_blocked), 0);
        }
        
        return;
    }

    if (user_action)
        user_action(prev_payload, prev_header->buf_len);
}

void rsdco_detect_action_rcvr(struct MemoryHotel* hotel, uint32_t header_room, uint32_t prev_header_room, void (*user_action)(void*, uint16_t)) {

    struct LogHeader* header = reinterpret_cast<struct LogHeader*>(&(hotel->room[header_room]));
    uint32_t* payload = &(hotel->room[header_room + 2]);

    // Key area not important, since it will not be used at ACK.
    if (header->message != RSDCO_HDR_ONLY) {
        // printf("Receiver: Mine! %ld Reply as ACK\n", header->hashed);

        rsdco_request_to_rpli(payload, header->buf_len, nullptr, 0, header->hashed, RSDCO_MSG_ACK);

        // User action must be replayed.
        if (user_action)
            (*user_action)(payload, header->buf_len);
    }
}
