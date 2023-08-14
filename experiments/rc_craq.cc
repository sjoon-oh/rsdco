
#include <vector>
#include <string>
#include <regex>

#include <random>

#include <cassert>
#include <cstdio>

#include <iostream>
#include <thread>

#include <cstdlib>
#include <unistd.h>

#include <pthread.h>

pthread_mutex_t propose_lock = PTHREAD_MUTEX_INITIALIZER;

#include "../hartebeest/src/includes/hartebeest-c.h"
#include "../src/includes/rsdco.h"

int is_head(int nid) {
    return (nid == 0);
}

std::string sysvar_nid;
std::string sysvar_participants;
std::vector<std::string> sysvar_nids;

int sysvar_nid_int;

struct ibv_mr* hotel_mr = nullptr;
struct ibv_qp* prev_qp = nullptr;
struct ibv_qp* next_qp = nullptr;

struct ibv_mr* remote_prev_mr = nullptr;
struct ibv_mr* remote_next_mr = nullptr;

struct ibv_qp* remote_prev_qp = nullptr;
struct ibv_qp* remote_next_qp = nullptr;

void alloc_register_resource() {

    sysvar_nid_int = std::stoi(hartebeest_get_sysvar("HARTEBEEST_NID"));
    printf("NID: %ld\n", sysvar_nid_int);

    hartebeest_init();
    rsdco_init_timers();

    sysvar_nid = std::string(hartebeest_get_sysvar("HARTEBEEST_NID"));
    sysvar_participants = std::string(hartebeest_get_sysvar("HARTEBEEST_PARTICIPANTS"));

    std::regex regex(",");
    sysvar_nids = std::vector<std::string>(
        std::sregex_token_iterator(sysvar_participants.begin(), sysvar_participants.end(), regex, -1),
        std::sregex_token_iterator());

    sysvar_nid_int = std::stoi(sysvar_nid);
    hartebeest_create_local_pd("rc-craq-pd");

    // MR (Single)
    std::string mr_key = "hotel-" + sysvar_nid;

    assert(
        hartebeest_create_local_mr(
            "rc-craq-pd", mr_key.c_str(), 
            RSDCO_BUFSZ, 
            IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE)
        == true);

    hotel_mr = hartebeest_get_local_mr("rc-craq-pd", mr_key.c_str());

    bool is_memc_pushed = hartebeest_memc_push_local_mr(
            mr_key.c_str(),
            "rc-craq-pd",
            mr_key.c_str()
        );
    assert(is_memc_pushed == true);

    std::string qp_key = "from-qp-" + sysvar_nid;
    hartebeest_create_basiccq(("scq-from-" + sysvar_nid).c_str());
    hartebeest_create_basiccq(("rcq-from-" + sysvar_nid).c_str());
    hartebeest_create_local_qp(
        "rc-craq-pd", qp_key.c_str(), IBV_QPT_RC, 
        ("scq-from-" + sysvar_nid).c_str(),
        ("rcq-from-" + sysvar_nid).c_str()
    );
    hartebeest_init_local_qp("rc-craq-pd", qp_key.c_str());
    prev_qp = hartebeest_get_local_qp("rc-craq-pd", qp_key.c_str());

    is_memc_pushed = hartebeest_memc_push_local_qp(qp_key.c_str(), "rc-craq-pd", qp_key.c_str());
    assert(is_memc_pushed == true);

    qp_key = "to-qp-" + sysvar_nid;
    hartebeest_create_basiccq(("scq-to-" + sysvar_nid).c_str());
    hartebeest_create_basiccq(("rcq-to-" + sysvar_nid).c_str());
    hartebeest_create_local_qp(
        "rc-craq-pd", qp_key.c_str(), IBV_QPT_RC, 
        ("scq-to-" + sysvar_nid).c_str(),
        ("rcq-to-" + sysvar_nid).c_str()
    );
    hartebeest_init_local_qp("rc-craq-pd", qp_key.c_str());
    next_qp = hartebeest_get_local_qp("rc-craq-pd", qp_key.c_str());

    is_memc_pushed = hartebeest_memc_push_local_qp(qp_key.c_str(), "rc-craq-pd", qp_key.c_str());
    assert(is_memc_pushed == true);
}

void get_remote_resource() {

    // Get all, including mine.
    for (auto& nid: sysvar_nids) {
        hartebeest_memc_fetch_remote_mr(("hotel-" + nid).c_str());
        hartebeest_memc_fetch_remote_qp(("from-qp-" + nid).c_str());
        hartebeest_memc_fetch_remote_qp(("to-qp-" + nid).c_str());
    }

    for (auto& nid: sysvar_nids) {

    }
}

void connect_to_from() {

    switch(sysvar_nid_int) {
        case 0:
            remote_next_mr = hartebeest_get_remote_mr("hotel-1");
            remote_prev_mr = nullptr;
            
            remote_next_qp = hartebeest_get_remote_qp("from-qp-1");
            remote_prev_qp = nullptr;

            hartebeest_connect_local_qp("rc-craq-pd", "to-qp-0", "from-qp-1");

            break;

        case 1:
            remote_next_mr = hartebeest_get_remote_mr("hotel-2");
            remote_prev_mr = hartebeest_get_remote_mr("hotel-0");
            
            remote_next_qp = hartebeest_get_remote_qp("from-qp-2");
            remote_prev_qp = hartebeest_get_remote_qp("to-qp-0");

            hartebeest_connect_local_qp("rc-craq-pd", "to-qp-1", "from-qp-2");
            hartebeest_connect_local_qp("rc-craq-pd", "from-qp-1", "to-qp-0");

            break;

        case 2:

            remote_next_mr = nullptr;
            remote_prev_mr = hartebeest_get_remote_mr("hotel-1");
            
            remote_next_qp = nullptr;
            remote_prev_qp = hartebeest_get_remote_qp("to-qp-1");

            hartebeest_connect_local_qp("rc-craq-pd", "from-qp-2", "to-qp-1");

            break;

        default:
            assert(false);
    }
}

void wait_for_others() {

    std::string done_msg = "qp-connected-" + sysvar_nid;
    hartebeest_memc_push_general(done_msg.c_str());
    
    for (auto& nid: sysvar_nids)
        hartebeest_memc_wait_general( ("qp-connected-" + nid).c_str());

    sleep(10);
    hartebeest_memc_del_general(done_msg.c_str());

    hartebeest_memc_del_general(("hotel-" + sysvar_nid).c_str());
    hartebeest_memc_del_general(("from-qp-" + sysvar_nid).c_str());
    hartebeest_memc_del_general(("to-qp-" + sysvar_nid).c_str());
}

void local_hotel_init() {
    struct MemoryHotel* hotel = 
        reinterpret_cast<struct MemoryHotel*>(hotel_mr->addr);

    hotel->room_free = RSDCO_AVAIL_ROOMS;
    hotel->next_room_free = 0;

    for (int idx = 0; idx < RSDCO_RESERVED; idx++)
        hotel->reserved[idx] = 0;

    std::memset(hotel->room, 0, sizeof(sizeof(uint64_t) * RSDCO_AVAIL_ROOMS));
}

void generate_random_str(std::mt19937& generator, char* buffer, int len) {

    // Printable ASCII range.
    const char* character_set = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    std::uniform_int_distribution<int> distributor(0, 61);
    for (int i = 0; i < len; i++)
        buffer[i] = character_set[static_cast<char>(distributor(generator))];
}

struct Payload {
    char buffer[4096];
};

void magic_test() {
    struct MemoryHotel* local_hotel = reinterpret_cast<struct MemoryHotel*>(hotel_mr->addr);
    switch(sysvar_nid_int) {
    case 0: {
        // struct MemoryHotel* remote_hotel = reinterpret_cast<struct MemoryHotel*>(remote_prev_mr->addr);

        while(local_hotel->reserved[0] != 0x00000003) ;
        printf("node1 ACKED !!\n");

        while(local_hotel->reserved[1] != 0x00000003) ; 
        printf("Both acked !!\n");
        printf("Magic test END\n");
    }
    break;
    case 1: {
        printf("Sending magic to reserved[0]...\n");

        struct MemoryHotel* remote_hotel = reinterpret_cast<struct MemoryHotel*>(remote_prev_mr->addr);
        local_hotel->reserved[0] = 0x00000003;

        hartebeest_rdma_post_single_fast(
            prev_qp, 
            &local_hotel->reserved[0], 
            &remote_hotel->reserved[0],
            4, IBV_WR_RDMA_WRITE, 
            hotel_mr->lkey, remote_prev_mr->rkey, 0
        );
        hartebeest_rdma_send_poll(prev_qp);

        printf("Waiting...\n");
        while(local_hotel->reserved[1] != 0x00000003) ;

        printf("Sending magic to reserved[1]...\n");
        hartebeest_rdma_post_single_fast(
            prev_qp, 
            &local_hotel->reserved[1], 
            &remote_hotel->reserved[1],
            4, IBV_WR_RDMA_WRITE, 
            hotel_mr->lkey, remote_prev_mr->rkey, 0
        );
        hartebeest_rdma_send_poll(prev_qp);
        printf("Magic send end\n");
    }
    break;
    case 2: {
        struct MemoryHotel* remote_hotel = reinterpret_cast<struct MemoryHotel*>(remote_prev_mr->addr);
        local_hotel->reserved[1] = 0x00000003;

        hartebeest_rdma_post_single_fast(
            prev_qp, 
            &local_hotel->reserved[1], 
            &remote_hotel->reserved[1],
            4, IBV_WR_RDMA_WRITE, 
            hotel_mr->lkey, remote_prev_mr->rkey, 0
        );
        hartebeest_rdma_send_poll(prev_qp);
        printf("Magic sent\n");
    }
    break;
    default:
        assert(false);
    }
}

struct __attribute__((packed)) LogHeaderCraq {
    uint16_t    buf_len;
    uint8_t     proposal;
    uint8_t     message;
};

//
void rcraq_propose(void* buf, uint16_t buf_len) {

    pthread_mutex_lock(&propose_lock);

    struct MemoryHotel* hotel = 
        reinterpret_cast<struct MemoryHotel*>(hotel_mr->addr);

    uint32_t header_room = hotel->next_room_free;
    struct LogHeaderCraq* header = reinterpret_cast<struct LogHeaderCraq*>(&(hotel->room[header_room]));

    hotel->next_room_free += 1;

    header->proposal = hotel->reserved[1];
    header->buf_len = buf_len;
    header->message = 0x02;

    uint32_t payload_room = hotel->next_room_free;
    uint32_t* payload = &(hotel->room[payload_room]);
    
    hotel->next_room_free = rsdco_next_free_room(payload_room, buf_len + sizeof(uint8_t));

    uintptr_t canary = reinterpret_cast<uintptr_t>(payload) + buf_len;
    uint8_t canary_val = 36;

    std::memcpy(payload, buf, buf_len);
    std::memcpy(reinterpret_cast<void*>(canary), &canary_val, sizeof(uint8_t));

    struct MemoryHotel* remote_hotel = reinterpret_cast<struct MemoryHotel*>(remote_next_mr->addr);
    void* remote_header = &(remote_hotel->room[header_room]);
    size_t write_sz = sizeof(struct LogHeaderCraq) + buf_len + sizeof(uint8_t);

    hartebeest_rdma_post_single_fast(
        next_qp, 
        header, 
        remote_header,
        write_sz, IBV_WR_RDMA_WRITE, 
        hotel_mr->lkey, remote_next_mr->rkey, 0
    );
    hartebeest_rdma_send_poll(next_qp);

    // printf("%s proposed: waiting...\n", (char*)buf);
    while (header->message != 0) {
        // Wait for ACK
    }
    // printf("%s : ACKED...\n", (char*)buf);

    hotel->reserved[1] += 1;

    pthread_mutex_unlock(&propose_lock);
}

void rcraq_wait() {

    printf("Into listening mode...\n");

    struct MemoryHotel* hotel = 
        reinterpret_cast<struct MemoryHotel*>(hotel_mr->addr);

    uint8_t local_proposal = hotel->reserved[1];
    uint32_t header_room, payload_room;

    struct LogHeaderCraq* header;
    uint32_t* payload;

    uintptr_t canary;
    uint8_t canary_val;

    while (1) {

        header_room = hotel->next_room_free;
        payload_room = hotel->next_room_free + 1;

        header = reinterpret_cast<struct LogHeaderCraq*>(&(hotel->room[header_room]));
        payload = &(hotel->room[payload_room]);

        canary = reinterpret_cast<uintptr_t>(payload) + header->buf_len;
        canary_val = *(reinterpret_cast<uint8_t*>(canary));

        // Detected
        if (
            ((header->proposal == local_proposal) && (canary_val == 36))
            ) {

            // printf("Detected: %s\n", (char*)payload);

            hotel->next_room_free = rsdco_next_free_room(payload_room, (header->buf_len + sizeof(uint8_t)));

            local_proposal = header->proposal + 1;

            // Propagate
            if (sysvar_nid_int != 2) {

                // printf("Propagating...\n");

                struct MemoryHotel* remote_hotel = reinterpret_cast<struct MemoryHotel*>(remote_next_mr->addr);
                struct LogHeaderCraq* remote_header = reinterpret_cast<struct LogHeaderCraq*>(&(remote_hotel->room[header_room]));
                size_t write_sz = sizeof(struct LogHeaderCraq) + header->buf_len + sizeof(uint8_t);

                hartebeest_rdma_post_single_fast(
                    next_qp, 
                    header, 
                    remote_header,
                    write_sz, IBV_WR_RDMA_WRITE, 
                    hotel_mr->lkey, remote_next_mr->rkey, 0
                );
                hartebeest_rdma_send_poll(next_qp);

                // printf("Waiting for ACK...\n");
                while (header->message != 0) {
                    // Wait for ACK
                }

                remote_hotel = reinterpret_cast<struct MemoryHotel*>(remote_prev_mr->addr);
                remote_header = reinterpret_cast<struct LogHeaderCraq*>(&(remote_hotel->room[header_room]));

                header->message = 0;

                hartebeest_rdma_post_single_fast(
                    prev_qp, 
                    &header->message, 
                    &remote_header->message,
                    1, IBV_WR_RDMA_WRITE, 
                    hotel_mr->lkey, remote_prev_mr->rkey, 0
                );
                hartebeest_rdma_send_poll(prev_qp);
                // printf("Received ACK, propagating ack.\n");
            }
            else {
                // printf("I am the last!...\n");

                struct MemoryHotel* remote_hotel = reinterpret_cast<struct MemoryHotel*>(remote_prev_mr->addr);
                struct LogHeaderCraq* remote_header = reinterpret_cast<struct LogHeaderCraq*>(&(remote_hotel->room[header_room]));

                header->message = 0;

                hartebeest_rdma_post_single_fast(
                    prev_qp, 
                    &header->message, 
                    &remote_header->message,
                    1, IBV_WR_RDMA_WRITE, 
                    hotel_mr->lkey, remote_prev_mr->rkey, 0
                );
                hartebeest_rdma_send_poll(prev_qp);

                //  printf("ACK Sent\n");
            }

            *(reinterpret_cast<uint8_t*>(canary)) = 0;
        }

    }
}


void replicate_func(std::mt19937& generator, size_t num_requests, int payload_sz, int key_sz) {
    struct Payload local_buffer;

    for (size_t nth_req = 0; nth_req < num_requests; nth_req++) {

        generate_random_str(generator, local_buffer.buffer, payload_sz);
        local_buffer.buffer[payload_sz - 1] = 0;

        uint64_t idx = rsdco_get_ts_start_rpli();

        rcraq_propose(&local_buffer, payload_sz);

        rsdco_get_ts_end_rpli(idx);

        if ((nth_req % 10000) == 0)
            printf("Count: %ld\n", nth_req);
    }
}


int main() {

    alloc_register_resource();
    get_remote_resource();
    
    connect_to_from();
    wait_for_others();
    local_hotel_init();
    
    assert(hotel_mr != nullptr);
    sleep(3);
    magic_test();

    char* fixed_buf = "asldfjalksdjfqwoiejfqowiejfoiasdjofijasoeifjawoiejfoaisdjf";
    size_t buf_len = strlen(fixed_buf);

    std::random_device random_device;
    std::mt19937 generator(random_device());
    size_t num_requests = 1000000;

    std::vector<std::thread> threads;
    if (sysvar_nid_int == 0) {
        for (int i = 0; i < 3; i++) {
            std::cout << "Thread spawn: " << i << "\n";
            threads.push_back(
                std::move(std::thread(replicate_func, std::ref(generator), num_requests / 3, 32, 16)));
        }

        for (int i = 0; i < 3; i++) {
            std::cout << "Thread waiting: " << i << "\n";
            threads.at(i).join();
        }
    }
    else
        rcraq_wait();

    elapsed_timer_record_manual();

    sleep(11);
    return 0;

}