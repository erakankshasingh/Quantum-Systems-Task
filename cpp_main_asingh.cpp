//main.cpp
#include "cpp_udpScheduler_asingh.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    try {
        UdpScheduler scheduler;

        // Demo payload: "hello"
        std::vector<std::uint8_t> msg{'h', 'e', 'l', 'l', 'o'};

        // 1) Send immediately
        bool ok = scheduler.send_now("127.0.0.1", 9000, msg);
        std::cout << "Immediate send: " << (ok ? "OK" : "FAILED") << std::endl;

        // 2) Send once after 3 seconds
        auto one_shot_handle = scheduler.send_after("127.0.0.1", 9000, msg, 3);
        std::cout << "Scheduled one-shot send with handle: " << one_shot_handle << std::endl;

        // 3) Send periodically every 2 seconds
        auto periodic_handle = scheduler.send_periodic("127.0.0.1", 9000, msg, 2);
        std::cout << "Started periodic send with handle: " << periodic_handle << std::endl;

        // Let periodic sending run for a while
        std::this_thread::sleep_for(std::chrono::seconds(7));

        // Cancel periodic sending
        scheduler.cancel(periodic_handle);
        std::cout << "Cancelled periodic send with handle: " << periodic_handle << std::endl;

        // Wait a little so it is easy to observe behavior
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "Demo finished." << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}