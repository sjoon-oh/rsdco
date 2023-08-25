
#include <thread>
#include <vector>

#include <iostream>

#include <infiniband/verbs.h>

#include "../hartebeest/src/includes/hartebeest-c.h"

#include "./includes/rsdco_reqs.h"
#include "./includes/rsdco_wrkr.h"

extern std::string sysvar_nid;
extern std::vector<std::string> rsdco_sysvar_nids;

extern std::string rsdco_common_pd;

extern int rsdco_sysvar_nid_int;
extern int mencius_propose_enable;

#define RSDCO_REGKEY(identifier, src, dst) \
    ((std::string(identifier) + "(" + std::string(src) + "->" + std::string(dst) + ")").c_str())

std::vector<std::string> rsdco_wrkr_name;

void rsdco_spawn_receiver(void (*user_action)(void*, uint16_t)) {

    std::string detector;
    struct ibv_mr* detector_mr;

    std::string mencius_unlocker_thread;

    int unlocked_by = (rsdco_sysvar_nid_int + 1) % rsdco_sysvar_nids.size();
    mencius_unlocker_thread = rsdco_sysvar_nids.at(unlocked_by);

    int mencius_enabled = 0;

    if (rsdco_sysvar_nid_int == 0) {
        __sync_fetch_and_add(&mencius_propose_enable, 1);
        std::cout << "Propose will start by me. " << rsdco_sysvar_nid_int << std::endl;
    }

    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        if (nid == mencius_unlocker_thread) {
            mencius_enabled = 1;
            std::cout << "Will be unlocked by " << nid << std::endl;
        }
        else
            mencius_enabled = 0;

        detector = RSDCO_REGKEY("replayer-mr", nid, sysvar_nid);
        detector_mr = hartebeest_get_local_mr(rsdco_common_pd.c_str(), detector.c_str());

        rsdco_wrkr_name.push_back(std::string(detector));

        std::thread wrkr(
                rsdco_detect_poll, 
                rsdco_wrkr_name.back().c_str(), detector_mr, 
                rsdco_detect_action_rply,
                user_action, mencius_enabled
            );
        wrkr.detach();
    }

    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;

        detector = RSDCO_REGKEY("receiver-mr", nid, sysvar_nid);
        detector_mr = hartebeest_get_local_mr(rsdco_common_pd.c_str(), detector.c_str());

        rsdco_wrkr_name.push_back(std::string(detector));
    
        std::thread wrkr(
                rsdco_detect_poll, 
                rsdco_wrkr_name.back().c_str(), detector_mr, 
                rsdco_detect_action_rcvr,
                user_action, 0
            );
        wrkr.detach();
    }
}

