
#include <thread>
#include <vector>

#include <infiniband/verbs.h>

#include "../hartebeest/src/includes/hartebeest-c.h"

#include "./includes/rsdco_reqs.h"
#include "./includes/rsdco_wrkr.h"

extern std::string sysvar_nid;
extern std::vector<std::string> rsdco_sysvar_nids;

extern std::string rsdco_common_pd;

#define RSDCO_REGKEY(identifier, src, dst) \
    ((std::string(identifier) + "(" + std::string(src) + "->" + std::string(dst) + ")").c_str())

std::vector<std::string> rsdco_wrkr_name;

void rsdco_spawn_receiver(void (*user_action)(void*, uint16_t)) {

    std::string detector;
    struct ibv_mr* detector_mr;
    
    for (auto& nid: rsdco_sysvar_nids) {
        if (nid == sysvar_nid)
            continue;
        detector = RSDCO_REGKEY("replayer-mr", nid, sysvar_nid);
        detector_mr = hartebeest_get_local_mr(rsdco_common_pd.c_str(), detector.c_str());

        rsdco_wrkr_name.push_back(std::string(detector));

        std::thread wrkr(
                rsdco_detect_poll, 
                rsdco_wrkr_name.back().c_str(), detector_mr, 
                rsdco_detect_action_rply,
                user_action
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
                user_action
            );
        wrkr.detach();
    }
}

