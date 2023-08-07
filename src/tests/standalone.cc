/* github.com/sjoon-oh/soren
 * @author: Sukjoon Oh, sjoon@kaist.ac.kr
 * 
 * Project SOREN
 */

#include <string>
#include <random>

#include <iostream>
#include <thread>

#include <unistd.h>

#include "../includes/rsdco.h"

void generate_random_str(std::mt19937& generator, char* buffer, int len) {

    // Printable ASCII range.
    const char* character_set = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    std::uniform_int_distribution<int> distributor(0, 61);
    for (int i = 0; i < len; i++)
        buffer[i] = character_set[static_cast<char>(distributor(generator))];
}

struct Payload {
    char buffer[4096];
};

void replicate_func(std::mt19937& generator, size_t num_requests, int payload_sz, int key_sz) {
    struct Payload local_buffer;
    for (size_t nth_req = 0; nth_req < num_requests; nth_req++) {

        generate_random_str(generator, local_buffer.buffer, payload_sz);
        local_buffer.buffer[payload_sz - 1] = 0;

        rsdco_add_request(
            local_buffer.buffer,
            payload_sz, 
            local_buffer.buffer,
            key_sz,
            0,
            rsdco_rule_skew_single
        );

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        do {
            clock_gettime(CLOCK_MONOTONIC, &end);
        } while((get_elapsed_nsec(end, start) < 10000));

        if ((nth_req % 10000) == 0)
            std::cout << "Count: " << nth_req << "\n";
    }

    std::cout << "Thread end\n";
}

int main(int argc, char *argv[]) {

    const int payload_sz = atoi(argv[1]);
    const int key_sz = atoi(argv[2]);
    const int n_threads = atoi(argv[3]);

    const size_t default_buf_sz = RSDCO_BUFSZ;
    
    // 2,147,483,647
    // 1,000,000
    
    size_t num_requests = 1000000; // Give some space
    size_t num_req_per_thread = num_requests / n_threads;

    std::cout << "Reqs per threads: " << num_req_per_thread << "\n";

    rsdco_connector_init();

    rsdco_local_hotel_init();
    
    rsdco_remote_writer_hotel_init();
    rsdco_remote_receiver_hotel_init();
    
    rsdco_spawn_receiver(nullptr);

    //
    // Initialize randomization device
    std::random_device random_device;
    std::mt19937 generator(random_device());

    std::vector<std::thread> threads;

    std::cout << "Standalone test start.\n";

    for (int i = 0; i < n_threads; i++) {
        std::cout << "Thread spawn: " << i << "\n";
        threads.push_back(
            std::move(std::thread(replicate_func, std::ref(generator), num_req_per_thread, payload_sz, key_sz)));
    }

    for (int i = 0; i < n_threads; i++) {
        std::cout << "Thread waiting: " << i << "\n";
        threads.at(i).join();
    }

    // replicate_func(generator, num_requests, payload_sz, key_sz);
    std::cout << "OK\n";

    for (int sec = 30; sec > 0; sec--) {
        std::cout <<"\rWaiting for: " << sec << " sec     " << std::flush;
        sleep(1);
    }

    elapsed_timer_record_manual();

    std::cout <<"Exit.\n";
    
    return 0;
}



