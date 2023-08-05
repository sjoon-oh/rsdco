
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <infiniband/verbs.h> // OFED IB verbs

#include "../includes/rsdco.h"
#include "../../hartebeest/src/includes/hartebeest-c.h"

int main() {

    rsdco_connector_init();

    uint32_t payload_sz = 64;

    rsdco_local_hotel_init();

    rsdco_remote_writer_hotel_init();
    rsdco_remote_receiver_hotel_init();

    rsdco_spawn_receiver(NULL);

    size_t n_req = RSDCO_BUFSZ / (120 + payload_sz); // Give some space

    int this_node_id = rsdco_get_node_id();
    if (this_node_id == 0) {
        
        char* predefined_str = "Literature adds to reality.";
        char payload[32] = { 0, };

        strcpy(payload, predefined_str);
        for (int i = 0; i < 200; i++) {
            rsdco_add_request(
                payload,
                32, 
                payload,
                16,
                0,
                rsdco_rule
            );
        }

        sleep(10);
        elapsed_timer_record_manual();

    }
    else {
        // rsdco_detect_poll("replayer-mr(0->1)");
    }

    return 0;
}


