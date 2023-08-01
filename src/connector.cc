
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
#include "./includes/connector.h"

std::string sysvar_nid;
std::string sysvar_participants;

std::vector<std::string> sysvar_nids;

std::string key_rsdco_pd = "rsdco-pd";

struct RsdcoWriteContext {
    struct ibv_qp* local_qp;
    struct ibv_qp* local_mr;
    struct ibv_mr* remote_mr;
};

const size_t mr_buf_sz = 2147483648;


void rsdco_writer_init(std::string prefix_writer) {

    bool is_memc_pushed = false;

    std::string mr_key = prefix_writer + "-mr-" + sysvar_nid;
    hartebeest_create_local_mr(
            key_rsdco_pd.c_str(),
            mr_key.c_str(), 
            mr_buf_sz,
            IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
        );
    
    for (auto& nid: sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        std::string qp_key = prefix_writer + "-qp-from-" + sysvar_nid + "-to-" + nid;
        std::string send_cq_key = prefix_writer + "-send-cq-to-" + nid;
        std::string recv_cq_key = prefix_writer + "-recv-cq-to-" + nid;

        hartebeest_create_basiccq(send_cq_key.c_str());
        hartebeest_create_basiccq(recv_cq_key.c_str());

        hartebeest_create_local_qp(
            key_rsdco_pd.c_str(), qp_key.c_str(), IBV_QPT_RC, send_cq_key.c_str(), recv_cq_key.c_str()
        );

        is_memc_pushed = hartebeest_memc_push_local_qp(
            qp_key.c_str(),
            key_rsdco_pd.c_str(),
            qp_key.c_str()
        );

        assert(is_memc_pushed == true);
    }
}

void rsdco_receiver_init(std::string prefix_receiver) {

    bool is_memc_pushed = false;

    for (auto& nid: sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        std::string mr_key = prefix_receiver + "-mr-from-" + nid + "-to-" + sysvar_nid;
        hartebeest_create_local_mr(
                key_rsdco_pd.c_str(),
                mr_key.c_str(), 
                mr_buf_sz,
                IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
            );

        hartebeest_memc_push_local_mr(
            mr_key.c_str(),
            key_rsdco_pd.c_str(),
            mr_key.c_str()
        );

        std::string qp_key = prefix_receiver + "-qp-from-" + nid + "-to-" + sysvar_nid;
        std::string send_cq_key = prefix_receiver + "-send-cq-from-" + nid;
        std::string recv_cq_key = prefix_receiver + "-recv-cq-from-" + nid;

        hartebeest_create_basiccq(send_cq_key.c_str());
        hartebeest_create_basiccq(recv_cq_key.c_str());

        hartebeest_create_local_qp(
            key_rsdco_pd.c_str(), qp_key.c_str(), IBV_QPT_RC, send_cq_key.c_str(), recv_cq_key.c_str()
        );

        is_memc_pushed = hartebeest_memc_push_local_qp(
            qp_key.c_str(),
            key_rsdco_pd.c_str(),
            qp_key.c_str()
        );

        assert(is_memc_pushed == true);
    }
}

void rsdco_writer_qp_connect(std::string prefix_writer, std::string prefix_receiver) {

    for (auto& nid: sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        std::string local_qp_key = prefix_writer + "-qp-from-" + sysvar_nid + "-to-" + nid;
        std::string remote_mr_key = prefix_receiver + "-mr-from-" + sysvar_nid + "-to-" + nid;
        std::string remote_qp_key = prefix_receiver + "-qp-from-" + sysvar_nid + "-to-" + nid;

        hartebeest_memc_fetch_remote_mr(remote_mr_key.c_str());
        hartebeest_memc_fetch_remote_qp(remote_qp_key.c_str());
        
        hartebeest_init_local_qp(key_rsdco_pd.c_str(), local_qp_key.c_str());
        hartebeest_connect_local_qp(key_rsdco_pd.c_str(), local_qp_key.c_str(), remote_qp_key.c_str());
    }
}

void rsdco_receiver_qp_connect(std::string prefix_receiver, std::string prefix_writer) {

    for (auto& nid: sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        std::string local_qp_key = prefix_receiver + "-qp-from-" + nid + "-to-" + sysvar_nid;
        std::string remote_qp_key = prefix_writer + "-qp-from-" + nid + "-to-" + sysvar_nid;

        hartebeest_memc_fetch_remote_qp(remote_qp_key.c_str());
        
        hartebeest_init_local_qp(key_rsdco_pd.c_str(), local_qp_key.c_str());
        hartebeest_connect_local_qp(key_rsdco_pd.c_str(), local_qp_key.c_str(), remote_qp_key.c_str());
    }
}

void rdsco_memc_clean(std::string prefix_writer, std::string prefix_receiver) {
    
    bool is_memc_del = false;
    
    // Clean up only pushed ones by this node
    for (auto& nid: sysvar_nids) {

        if (nid == sysvar_nid)
            continue;

        std::string qp_key_1 = prefix_writer + "-qp-from-" + sysvar_nid + "-to-" + nid;
        std::string qp_key_2 = prefix_receiver + "-qp-from-" + nid + "-to-" + sysvar_nid;
        std::string mr_key = prefix_receiver + "-mr-from-" + nid + "-to-" + sysvar_nid;

        is_memc_del = hartebeest_memc_del_general(qp_key_1.c_str());
        assert(is_memc_del == true);
        
        is_memc_del = hartebeest_memc_del_general(qp_key_2.c_str());
        assert(is_memc_del == true);
        
        is_memc_del = hartebeest_memc_del_general(mr_key.c_str());
        assert(is_memc_del == true);
    }
}

void rsdco_connector_init() {
    
    hartebeest_init();

    sysvar_nid = std::string(hartebeest_get_sysvar("HARTEBEEST_NID"));
    sysvar_participants = std::string(hartebeest_get_sysvar("HARTEBEEST_PARTICIPANTS"));

    std::regex regex(",");
    sysvar_nids = std::vector<std::string>(
        std::sregex_token_iterator(sysvar_participants.begin(), sysvar_participants.end(), regex, -1),
        std::sregex_token_iterator());

    hartebeest_create_local_pd(key_rsdco_pd.c_str());

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

    for (auto& nid: sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        std::string done_msg = "qp-connected-" + nid;
        hartebeest_memc_wait_general(done_msg.c_str());
    }

    rdsco_memc_clean("replicator", "replayer");
    rdsco_memc_clean("checker", "receiver");

    hartebeest_memc_del_general(msg.c_str());

    return;
}

