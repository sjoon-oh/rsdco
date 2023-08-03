
#include <stdio.h>
#include <assert.h>

#include <infiniband/verbs.h> // OFED IB verbs

#include "../includes/rsdco.h"
#include "../../hartebeest/src/includes/hartebeest-c.h"

int main() {

    rsdco_connector_init();

    uint32_t payload_sz = 64;

    rsdco_local_hotel_init();

    rsdco_remote_writer_hotel_init();
    rsdco_remote_receiver_hotel_init();

    struct elapsed_timer* local_timer = elapsed_timer_register();

    size_t n_req = RSDCO_BUFSZ / (120 + payload_sz); // Give some space

    int this_node_id = rsdco_get_node_id();
    if (this_node_id == 0) {
        
        char* predefined_str = "Literature adds to reality.";
        char payload[32] = { 0, };

        strcpy(payload, predefined_str);
        for (int i = 0; i < 200; i++) {

            uint64_t ts_idx = __sync_fetch_and_add(&(local_timer->ts_idx), 1);
            get_start_ts(local_timer, ts_idx);

            rsdco_add_request_rpli(payload, 32, payload + 2, 2, 0);
            get_end_ts(local_timer, ts_idx);
        }

        sleep(10);
        elapsed_timer_record_manual();

    }
    else {
        rsdco_replayer_detect("replayer-mr(0->1)");
    }

    return 0;
}


