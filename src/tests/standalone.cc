/* github.com/sjoon-oh/soren
 * @author: Sukjoon Oh, sjoon@kaist.ac.kr
 * 
 * Project SOREN
 */

#include <string>
#include <random>

#include <iostream>

#include <unistd.h>

#include "../includes/rsdco.h"

void generate_random_str(std::mt19937& generator, char* buffer, int len) {

    // Printable ASCII range.
    const char* character_set = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    std::uniform_int_distribution<int> distributor(0, 61);
    for (int i = 0; i < len; i++)
        buffer[i] = character_set[static_cast<char>(distributor(generator))];
}

extern 

int main(int argc, char *argv[]) {

    const int max_payload_sz = 512;

    const int payload_sz = atoi(argv[1]);
    const int key_sz = atoi(argv[2]);

    const size_t default_buf_sz = RSDCO_BUFSZ;
    
    size_t num_requests = 1000000; // Give some space

    struct Payload {
        char buffer[max_payload_sz];
    } local_buffer;

    rsdco_connector_init();

    rsdco_local_hotel_init();
    
    rsdco_remote_writer_hotel_init();
    rsdco_remote_receiver_hotel_init();

    //
    // Initialize randomization device
    std::random_device random_device;
    std::mt19937 generator(random_device());

    struct elapsed_timer* standalone_timer = elapsed_timer_register();

    std::cout << "Standalone test start.\n";

    for (size_t nth_req = 0; nth_req < num_requests; nth_req++) {

        generate_random_str(generator, local_buffer.buffer, payload_sz);
        local_buffer.buffer[payload_sz - 1] = 0;

        uint64_t ts_idx = __sync_fetch_and_add(&(standalone_timer->ts_idx), 1);
        get_start_ts(standalone_timer, ts_idx);

        rsdco_add_request_rpli(
            local_buffer.buffer,
            payload_sz, 
            local_buffer.buffer,
            key_sz,
            0
        );

        get_end_ts(standalone_timer, ts_idx);
    }

    std::cout << "\n";

    for (int sec = 30; sec > 0; sec--) {
        std::cout <<"\rWaiting for: " << sec << " sec     " << std::flush;
        sleep(1);
    }

    elapsed_timer_record_manual();

    std::cout <<"Exit.\n";
    
    return 0;
}



