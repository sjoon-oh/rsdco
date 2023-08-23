#include <pthread.h>

#include <vector>
#include <memory>
#include <iostream>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include <thread>

#include <unistd.h>

#include <infiniband/verbs.h>

#include "../hartebeest/src/includes/hartebeest-c.h"

#include "./includes/rsdco_conn.h"
#include "./includes/rsdco_heartbeat.h"

extern std::string sysvar_nid;
extern std::string sysvar_participants;
extern std::vector<std::string> rsdco_sysvar_nids;

extern int rsdco_sysvar_nid_int;

struct HeartbeatStatTable* hbstat;
uint32_t hbstat_lkey;
struct HeartbeatLocalTable local_hb_tracker;

std::vector<struct RsdcoConnectPair> rsdco_hb_conn{};

extern std::vector<struct RsdcoConnectPair> rsdco_rpli_conn;
extern std::vector<struct RsdcoConnectPair> rsdco_chkr_conn;

uint32_t rsdco_on_configure = 0;

// Predefined
extern std::string rsdco_common_pd;

const char* rsdco_make_regkey_2(std::string identifier, std::string src, std::string dst) {
    return (identifier + "(" + src + "->" + dst + ")").c_str();
}

void rsdco_heartbeat_qp_init() {
    
    bool is_memc_pushed = false;

    std::string qp_key;
    std::string cq_key;

    for (auto& nid: rsdco_sysvar_nids) {

        if (nid == sysvar_nid)
            continue;

        qp_key = rsdco_make_regkey_2("hbstat-table-qp-actv", sysvar_nid, nid);    // Active
        cq_key = rsdco_make_regkey_2("hbstat-table-cq-actv", sysvar_nid, nid);

        hartebeest_create_basiccq(cq_key.c_str());
        hartebeest_create_local_qp(
            rsdco_common_pd.c_str(), qp_key.c_str(), IBV_QPT_RC, cq_key.c_str(), cq_key.c_str()
        );

        is_memc_pushed = hartebeest_memc_push_local_qp(
            qp_key.c_str(), rsdco_common_pd.c_str(), qp_key.c_str()
        );
        assert(is_memc_pushed == true);

        qp_key = rsdco_make_regkey_2("hbstat-table-qp-pssv", nid, sysvar_nid);    // Passive
        cq_key = rsdco_make_regkey_2("hbstat-table-cq-pssv", nid, sysvar_nid);

        hartebeest_create_basiccq(cq_key.c_str());
        hartebeest_create_local_qp(
            rsdco_common_pd.c_str(), qp_key.c_str(), IBV_QPT_RC, cq_key.c_str(), cq_key.c_str()
        );

        is_memc_pushed = hartebeest_memc_push_local_qp(
            qp_key.c_str(), rsdco_common_pd.c_str(), qp_key.c_str()
        );
        assert(is_memc_pushed == true);
    }

    // Connect, Active
    for (auto& nid: rsdco_sysvar_nids) {

        if (nid == sysvar_nid)
            continue;

        std::string local_qp_key = rsdco_make_regkey_2("hbstat-table-qp-actv", sysvar_nid, nid);
        std::string remote_mr_key = "hbstat-table-" + nid;
        std::string remote_qp_key = rsdco_make_regkey_2("hbstat-table-qp-pssv", sysvar_nid, nid);

        hartebeest_memc_fetch_remote_mr(remote_mr_key.c_str());
        hartebeest_memc_fetch_remote_qp(remote_qp_key.c_str());

        rsdco_hb_conn.push_back(
            {
                .local_qp = hartebeest_get_local_qp(rsdco_common_pd.c_str(), local_qp_key.c_str()),
                .remote_mr = hartebeest_get_remote_mr(remote_mr_key.c_str()),
                .remote_node_id = std::stoi(nid)
            }
        );

        hartebeest_init_local_qp(rsdco_common_pd.c_str(), local_qp_key.c_str());
        hartebeest_connect_local_qp(rsdco_common_pd.c_str(), local_qp_key.c_str(), remote_qp_key.c_str());
    }

    // Passive connection.
    for (auto& nid: rsdco_sysvar_nids) {

        if (nid == sysvar_nid)
            continue;

        std::string local_qp_key = rsdco_make_regkey_2("hbstat-table-qp-pssv", nid, sysvar_nid);
        std::string remote_qp_key = rsdco_make_regkey_2("hbstat-table-qp-actv", nid, sysvar_nid);

        hartebeest_memc_fetch_remote_qp(remote_qp_key.c_str());

        hartebeest_init_local_qp(rsdco_common_pd.c_str(), local_qp_key.c_str());
        hartebeest_connect_local_qp(rsdco_common_pd.c_str(), local_qp_key.c_str(), remote_qp_key.c_str());
    }

    std::string msg = "hbqp-connected-" + sysvar_nid;
    hartebeest_memc_push_general(msg.c_str());

    for (auto& nid: rsdco_sysvar_nids) {
        msg = "hbqp-connected-" + nid;
        hartebeest_memc_wait_general(msg.c_str());
    }

    sleep(3);
    printf("Heartbeat connection OK.\n");

    msg = "hbqp-connected-" + sysvar_nid;
    hartebeest_memc_del_general(msg.c_str());

    // Clean up
    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;
        hartebeest_memc_del_general(rsdco_make_regkey_2("hbstat-table-qp-actv", sysvar_nid, nid));
        hartebeest_memc_del_general(rsdco_make_regkey_2("hbstat-table-qp-pssv", nid, sysvar_nid));
    }
}

void rsdco_heartbeat_init() {

    // No need to be atomically updated, for now.
    rsdco_on_configure = 0;
    
    assert(hartebeest_get_local_pd(rsdco_common_pd.c_str()) != nullptr);

    // Create local mr area for heartbeat signals
    assert(hartebeest_create_local_mr(
        rsdco_common_pd.c_str(), 
        ("hbstat-table-" + sysvar_nid).c_str(), 
        sizeof(struct HeartbeatStatTable),
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
        ) == true);
    
    auto local_hbstat_mr = hartebeest_get_local_mr(rsdco_common_pd.c_str(), ("hbstat-table-" + sysvar_nid).c_str());
    assert(local_hbstat_mr != nullptr);

    hbstat = reinterpret_cast<struct HeartbeatStatTable*>(local_hbstat_mr->addr);
    hbstat_lkey = local_hbstat_mr->lkey;

    hbstat->view = 0;
    hbstat->exclude = EXCLUDE_RESET;
    hbstat->msg = MSG_WAIT;

    std::memset(hbstat->live_status, 0, sizeof(hbstat->live_status));
    std::memset(hbstat->neon_sign, 0, sizeof(hbstat->neon_sign));

    for (int i = 0; i < MAX_N_NODES; i++) {
        local_hb_tracker.neon_sign[i] = -1;
        local_hb_tracker.score[i] = SCORE_DEFAULT;
    }

    hartebeest_memc_push_local_mr(
        ("hbstat-table-" + sysvar_nid).c_str(), rsdco_common_pd.c_str(), ("hbstat-table-" + sysvar_nid).c_str());

    // Make QP.
    rsdco_heartbeat_qp_init();
    hartebeest_memc_del_general(("hbstat-table-" + sysvar_nid).c_str()); // Remove MR

    printf("HB Conns: \n");
    for (auto& conn: rsdco_hb_conn) {
        printf("    - NID %ld, MR: %p, rkey: %ld\n", conn.remote_node_id, conn.remote_mr->addr, conn.remote_mr->rkey);
    }
}

void rsdco_incr_local_neonsign() {    
    hbstat->neon_sign[rsdco_sysvar_nid_int]++;
}

bool rsdco_is_lowest_alive() {

    int alive_lowest = 0;
    for (int i = 0; i < MAX_N_NODES; i++) {
        if (hbstat->live_status[i] == LIVE_STATUS_ALIVE) {
            alive_lowest = i;
            break;
        }
    }

    if (alive_lowest == rsdco_sysvar_nid_int)
        return true;
    else
        return false;
}

void rsdco_heartbeat_release() {

    // Initiate Heartbeat 
    if (rsdco_is_lowest_alive()) {

        hbstat->view = 1;
        hbstat->exclude = rsdco_sysvar_nid_int;
        hbstat->msg = MSG_RELEASE;

        for (auto& ctx: rsdco_hb_conn) {

            struct HeartbeatStatTable* remote_hbstat = 
                reinterpret_cast<struct HeartbeatStatTable*>(ctx.remote_mr->addr);

            hartebeest_rdma_post_single_fast(
                ctx.local_qp, hbstat, ctx.remote_mr->addr, 3 * sizeof(uint32_t), IBV_WR_RDMA_WRITE, 
                    hbstat_lkey, ctx.remote_mr->rkey, 0
            );

            hartebeest_rdma_send_poll(ctx.local_qp);
        }
    }
    else {
        printf("Waiting...\n");
        while (hbstat->msg != MSG_RELEASE) {
            ;
        }
    }
    
    printf("Heartbeat status table initialized.\n");
}

void rsdco_conn_remove(int dead_node_id) {

    for (std::vector<struct RsdcoConnectPair>::iterator it = rsdco_hb_conn.begin(); it != rsdco_hb_conn.end(); ++it) {
        if ((*it).remote_node_id == dead_node_id) {
            it = rsdco_hb_conn.erase(it);
            break;
        }
    }

    for (std::vector<struct RsdcoConnectPair>::iterator it = rsdco_rpli_conn.begin(); it != rsdco_rpli_conn.end(); ++it) {
        if ((*it).remote_node_id == dead_node_id) {
            it = rsdco_rpli_conn.erase(it);
            break;
        }
    }

    for (std::vector<struct RsdcoConnectPair>::iterator it = rsdco_chkr_conn.begin(); it != rsdco_chkr_conn.end(); ++it) {
        if ((*it).remote_node_id == dead_node_id) {
            it = rsdco_chkr_conn.erase(it);
            break;
        }
    }

    rsdco_sysvar_nids.pop_back(); 

    printf("Reset conns: <%ld, %ld, %ld>\n", rsdco_hb_conn.size(), rsdco_rpli_conn.size(), rsdco_chkr_conn.size());
}

void rsdco_reconfigure() {

    assert(hbstat->exclude != EXCLUDE_RESET);

    // Reconfigure
    __sync_fetch_and_add(&rsdco_on_configure, 1); // Lock globally.

    rsdco_conn_remove(hbstat->exclude);

    if (rsdco_is_lowest_alive()) {

        printf("Notifying NID[%ld] is dead.\n", hbstat->exclude);

        hbstat->view += 1;
        hbstat->exclude = EXCLUDE_RESET;
        hbstat->msg = MSG_RELEASE;

        for (auto& ctx: rsdco_hb_conn) {

            struct HeartbeatStatTable* remote_hbstat = 
                reinterpret_cast<struct HeartbeatStatTable*>(ctx.remote_mr->addr);

            hartebeest_rdma_post_single_fast(
                ctx.local_qp, hbstat, remote_hbstat, 3 * sizeof(uint32_t), IBV_WR_RDMA_WRITE, 
                    hbstat_lkey, ctx.remote_mr->rkey, 0
            );

            hartebeest_rdma_send_poll(ctx.local_qp);
        }
    }
    else {
        printf("Detected to be dead: %ld, Waiting for reconfigure...\n", hbstat->exclude);
        while (hbstat->msg != MSG_RELEASE) {
            ;
        }
    }
    printf("Reconfigured: excluded %ld\n", hbstat->exclude);

    hbstat->exclude = EXCLUDE_RESET;

    for (int i = 0; i < MAX_N_NODES; i++)
        local_hb_tracker.score[i] -= 1000;

    __sync_fetch_and_and(&rsdco_on_configure, 0);
}

bool rsdco_detect_liveness() {

    bool detected = false;
    for (auto& conn: rsdco_hb_conn) {
        
        if (hbstat->neon_sign[conn.remote_node_id] != local_hb_tracker.neon_sign[conn.remote_node_id]) {

            local_hb_tracker.neon_sign[conn.remote_node_id] = hbstat->neon_sign[conn.remote_node_id];   // Backup
            local_hb_tracker.score[conn.remote_node_id] = SCORE_DEFAULT;
        }
        else {
            if (local_hb_tracker.score[conn.remote_node_id] < 0) {
                hbstat->live_status[conn.remote_node_id] = LIVE_STATUS_DEAD;
                hbstat->msg = MSG_WAIT;
                hbstat->exclude = conn.remote_node_id;
                detected = true;

                break;
            }
            else {
                // Give more chance
                local_hb_tracker.score[conn.remote_node_id] -= 1;
            }
        }
    }

    return detected;
}

void rsdco_spawn_hb_tracker() {
    
    std::thread tracker([]() {
        
        while (1) {
            usleep(HEARTBEAT_UPDATE_US);
            rsdco_incr_local_neonsign();
            
            // Propagate
            for (auto& ctx: rsdco_hb_conn) {

                struct HeartbeatStatTable*  remote_hbstat= 
                    reinterpret_cast<struct HeartbeatStatTable*>(ctx.remote_mr->addr);

                hartebeest_rdma_post_single_fast(
                    ctx.local_qp, 
                    &hbstat->neon_sign[rsdco_sysvar_nid_int], 
                    &remote_hbstat->neon_sign[rsdco_sysvar_nid_int], sizeof(uint32_t), IBV_WR_RDMA_WRITE, 
                        hbstat_lkey, ctx.remote_mr->rkey, 0
                );

                hartebeest_rdma_send_poll(ctx.local_qp);
            }

            if (rsdco_detect_liveness())
                rsdco_reconfigure();

        }
    });
    
    tracker.detach();
}