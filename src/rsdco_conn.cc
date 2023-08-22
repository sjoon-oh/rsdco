
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <regex>
#include <string>
#include <iostream>
#include <vector>

#include <infiniband/verbs.h>

#include "../hartebeest/src/includes/hartebeest-c.h"

#include "./includes/rsdco_conn.h"
#include "./includes/rsdco_reqs.h"

#include "./includes/rsdco.h"

// #include ""


// System Vars
std::string sysvar_nid;
std::string sysvar_participants;
std::vector<std::string> rsdco_sysvar_nids;

int rsdco_sysvar_nid_int;

// Predefined
std::string rsdco_common_pd = "rsdco-pd";

// 
struct ibv_mr* rsdco_rpli_mr;
struct ibv_mr* rsdco_chkr_mr;

std::vector<struct RsdcoConnectPair> rsdco_rpli_conn{};
std::vector<struct RsdcoConnectPair> rsdco_chkr_conn{};


const int rsdco_get_node_id() {
    return rsdco_sysvar_nid_int;
}

const char* rsdco_make_regkey(std::string identifier, std::string src, std::string dst) {
    return (identifier + "(" + src + "->" + dst + ")").c_str();
}

void rsdco_writer_init(std::string prefix_writer) {

    bool is_memc_pushed = false;

    std::string mr_key = prefix_writer + "-mr-" + sysvar_nid;
    assert(hartebeest_create_local_mr(
            rsdco_common_pd.c_str(),
            mr_key.c_str(), 
            RSDCO_BUFSZ,
            IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
        ) == true);
    
    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        std::string qp_key = rsdco_make_regkey(prefix_writer + "-qp", sysvar_nid, nid);

        std::string send_cq_key = rsdco_make_regkey(prefix_writer + "-cq", sysvar_nid, nid);
        std::string recv_cq_key = rsdco_make_regkey(prefix_writer + "-cq", sysvar_nid, nid);

        hartebeest_create_basiccq(send_cq_key.c_str());
        hartebeest_create_basiccq(recv_cq_key.c_str());

        hartebeest_create_local_qp(
            rsdco_common_pd.c_str(), qp_key.c_str(), IBV_QPT_RC, send_cq_key.c_str(), recv_cq_key.c_str()
        );

        is_memc_pushed = hartebeest_memc_push_local_qp(
            qp_key.c_str(), rsdco_common_pd.c_str(), qp_key.c_str()
        );

        assert(is_memc_pushed == true);
    }
}

void rsdco_receiver_init(std::string prefix_receiver) {

    bool is_memc_pushed = false;

    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        // std::string mr_key = rsdco_make_mr_key(prefix_receiver, nid, sysvar_nid);
        std::string mr_key = rsdco_make_regkey(prefix_receiver + "-mr", nid, sysvar_nid);
        
        assert(hartebeest_create_local_mr(
                rsdco_common_pd.c_str(),
                mr_key.c_str(), 
                RSDCO_BUFSZ,
                IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
            ) == true);

        hartebeest_memc_push_local_mr(
            mr_key.c_str(),
            rsdco_common_pd.c_str(),
            mr_key.c_str()
        );

        std::string qp_key = rsdco_make_regkey(prefix_receiver + "-qp", nid, sysvar_nid);
        std::string send_cq_key = rsdco_make_regkey(prefix_receiver + "-cq", nid, sysvar_nid);
        std::string recv_cq_key = rsdco_make_regkey(prefix_receiver + "-cq", nid, sysvar_nid);

        hartebeest_create_basiccq(send_cq_key.c_str());
        hartebeest_create_basiccq(recv_cq_key.c_str());

        hartebeest_create_local_qp(
            rsdco_common_pd.c_str(), qp_key.c_str(), IBV_QPT_RC, send_cq_key.c_str(), recv_cq_key.c_str()
        );

        is_memc_pushed = hartebeest_memc_push_local_qp(
            qp_key.c_str(), rsdco_common_pd.c_str(), qp_key.c_str()
        );

        assert(is_memc_pushed == true);
    }
}

void rsdco_writer_qp_connect(std::string prefix_writer, std::string prefix_receiver) {

    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        std::string local_qp_key = rsdco_make_regkey(prefix_writer + "-qp", sysvar_nid, nid);
        std::string remote_mr_key = rsdco_make_regkey(prefix_receiver + "-mr", sysvar_nid, nid);
        std::string remote_qp_key = rsdco_make_regkey(prefix_receiver + "-qp", sysvar_nid, nid);

        hartebeest_memc_fetch_remote_mr(remote_mr_key.c_str());
        hartebeest_get_remote_mr(
            remote_mr_key.c_str()
        );
        hartebeest_memc_fetch_remote_qp(remote_qp_key.c_str());
        hartebeest_get_remote_qp(
            remote_qp_key.c_str()
        );

        hartebeest_init_local_qp(rsdco_common_pd.c_str(), local_qp_key.c_str());
        hartebeest_connect_local_qp(rsdco_common_pd.c_str(), local_qp_key.c_str(), remote_qp_key.c_str());
    }
}

void rsdco_receiver_qp_connect(std::string prefix_receiver, std::string prefix_writer) {

    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        std::string local_qp_key = rsdco_make_regkey(prefix_receiver + "-qp", nid, sysvar_nid);
        std::string remote_qp_key = rsdco_make_regkey(prefix_writer + "-qp", nid, sysvar_nid);

        hartebeest_memc_fetch_remote_qp(remote_qp_key.c_str());
        
        hartebeest_init_local_qp(rsdco_common_pd.c_str(), local_qp_key.c_str());
        hartebeest_connect_local_qp(rsdco_common_pd.c_str(), local_qp_key.c_str(), remote_qp_key.c_str());
    }
}

void rdsco_memc_clean(std::string prefix_writer, std::string prefix_receiver) {
    
    bool is_memc_del = false;
    
    // Clean up only pushed ones by this node
    for (auto& nid: rsdco_sysvar_nids) {

        if (nid == sysvar_nid)
            continue;

        std::string qp_key_1 = rsdco_make_regkey(prefix_writer + "-qp", sysvar_nid, nid);
        std::string qp_key_2 = rsdco_make_regkey(prefix_receiver + "-qp", nid, sysvar_nid);
        std::string mr_key = rsdco_make_regkey(prefix_receiver + "-mr", nid, sysvar_nid);

        is_memc_del = hartebeest_memc_del_general(qp_key_1.c_str());
        assert(is_memc_del == true);
        
        is_memc_del = hartebeest_memc_del_general(qp_key_2.c_str());
        assert(is_memc_del == true);
        
        is_memc_del = hartebeest_memc_del_general(mr_key.c_str());
        assert(is_memc_del == true);
    }
}

void rsdco_connector_init() {
    
    rsdco_rpli_mr = nullptr;
    rsdco_chkr_mr = nullptr;

    hartebeest_init();
    rsdco_init_timers();

    sysvar_nid = std::string(hartebeest_get_sysvar("HARTEBEEST_NID"));
    sysvar_participants = std::string(hartebeest_get_sysvar("HARTEBEEST_PARTICIPANTS"));

    std::regex regex(",");
    rsdco_sysvar_nids = std::vector<std::string>(
        std::sregex_token_iterator(sysvar_participants.begin(), sysvar_participants.end(), regex, -1),
        std::sregex_token_iterator());

    rsdco_sysvar_nid_int = std::stoi(sysvar_nid);
    hartebeest_create_local_pd(rsdco_common_pd.c_str());

    rsdco_writer_init("replicator");
    rsdco_writer_init("checker");
    
    rsdco_receiver_init("replayer");
    rsdco_receiver_init("receiver");

    rsdco_writer_qp_connect("replicator", "replayer");
    rsdco_writer_qp_connect("checker", "receiver");

    rsdco_receiver_qp_connect("replayer", "replicator");
    rsdco_receiver_qp_connect("receiver", "checker");

    std::string msg = "qp-connected-" + sysvar_nid;
    hartebeest_memc_push_general(msg.c_str());

    rsdco_rpli_mr = hartebeest_get_local_mr(rsdco_common_pd.c_str(), std::string("replicator-mr-" + sysvar_nid).c_str());
    rsdco_chkr_mr = hartebeest_get_local_mr(rsdco_common_pd.c_str(), std::string("checker-mr-" + sysvar_nid).c_str());

    assert(rsdco_rpli_mr != nullptr);
    assert(rsdco_chkr_mr != nullptr);

    struct ibv_qp* local_qp;
    struct ibv_mr* remote_mr;

    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;
        // std::cout << rsdco_make_regkey("replayer-mr", sysvar_nid, nid) << "\n";
        // std::cout << hartebeest_get_remote_mr(rsdco_make_regkey("replayer-mr", sysvar_nid, nid)) << "\n";
        local_qp = hartebeest_get_local_qp(rsdco_common_pd.c_str(), rsdco_make_regkey("replicator-qp", sysvar_nid, nid));
        remote_mr = hartebeest_get_remote_mr(rsdco_make_regkey("replayer-mr", sysvar_nid, nid));

        assert(local_qp != nullptr);
        assert(remote_mr != nullptr);

        rsdco_rpli_conn.push_back(
            {
                .local_qp = local_qp,
                .remote_mr = remote_mr,
                .remote_node_id = std::stoi(nid)
            }
        );

        local_qp = hartebeest_get_local_qp(rsdco_common_pd.c_str(), rsdco_make_regkey("checker-qp", sysvar_nid, nid));
        remote_mr = hartebeest_get_remote_mr(rsdco_make_regkey("receiver-mr", sysvar_nid, nid));
        
        assert(local_qp != nullptr);
        assert(remote_mr != nullptr);

        rsdco_chkr_conn.push_back(
            {
                .local_qp = local_qp,
                .remote_mr = remote_mr,
                .remote_node_id = std::stoi(nid)
            }
        );

        std::string done_msg = "qp-connected-" + nid;
        hartebeest_memc_wait_general(done_msg.c_str());
    }

    rdsco_memc_clean("replicator", "replayer");
    rdsco_memc_clean("checker", "receiver");

    hartebeest_memc_del_general(msg.c_str());

    // Added for Heartbeat
    rsdco_heartbeat_init();
    rsdco_heartbeat_release();

    return;
}

