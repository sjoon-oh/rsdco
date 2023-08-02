
#include <stdio.h>

#include "../includes/rsdco.h"


int main() {

    rsdco_connector_init();

    uint32_t next_room = rsdco_get_next_free_room(0, 120);
    printf("next room %d\n", next_room);

    rsdco_local_writer_memory_frame_init();

    return 0;
}


