

#include <iostream>

#include "../includes/rsdco.h"
// #include "./mica-herd/includes/mica.h"

// struct mica_kv hash_table;

int main() {
    
    // Raw
    rsdco_connector_init();
    rsdco_local_hotel_init();
    
    rsdco_remote_writer_hotel_init();
    rsdco_remote_receiver_hotel_init();
    
    rsdco_spawn_receiver(nullptr);


    int node_id = rsdco_get_node_id();

    printf("Node ID: %ld\n", node_id);

    // mica_init(&hash_table, node_id, 0, 2 * 1024 * 1024, 1024 * 1024 * 1024);



    return 0;
}






