#include <chrono>
#include <atomic>
#include <thread>
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace std::chrono_literals;

template <class F>
void StartThread(
    std::thread& thread,
    std::atomic<bool>& running,
    F&& process,
    std::chrono::seconds timeout)
{
    // (6) prevent std::terminate on overwrite
    if (thread.joinable()) {
        throw std::logic_error("StartThread called with joinable thread");
    }

    // (2) capture callable by value (no dangling reference)
    // (4) use steady_clock (monotonic)
    thread = std::thread(
        [&, proc = std::forward<F>(process), timeout]() mutable
        {
            const auto start = std::chrono::steady_clock::now();

            while (running.load(std::memory_order_relaxed))
            {
                // NOTE: (3) cooperative cancellation: process must return regularly
                const bool aborted = proc();

                // (5) compare durations directly without awkward casts
                if (aborted || (std::chrono::steady_clock::now() - start) > timeout)
                {
                    running.store(false, std::memory_order_relaxed);
                    break;
                }
            }
        });
}

int main()
{
    std::atomic<bool> my_running{true};
    std::thread my_thread1, my_thread2;

    // (1) make counters thread-safe
    std::atomic<int> loop_counter1{0};
    std::atomic<int> loop_counter2{0};

    StartThread(
        my_thread1,
        my_running,
        [&]() -> bool
        {
            // (3) still cooperative: sleep is not interruptible; keep it short-ish
            std::this_thread::sleep_for(2s);
            loop_counter1.fetch_add(1, std::memory_order_relaxed);
            return false;
        },
        10s);

    StartThread(
        my_thread2,
        my_running,
        [&]() -> bool
        {
            if (loop_counter2.load(std::memory_order_relaxed) < 5)
            {
                std::this_thread::sleep_for(1s);
                loop_counter2.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
            return true; // abort run
        },
        10s);

    my_thread1.join();
    my_thread2.join();

    std::cout << "C1: " << loop_counter1.load()
              << " C2: " << loop_counter2.load()
              << std::endl;
}