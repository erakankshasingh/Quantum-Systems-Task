#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

class UdpScheduler {
public:
    using Handle = std::uint64_t;
    using Clock  = std::chrono::steady_clock;

    UdpScheduler();
    ~UdpScheduler();

    UdpScheduler(const UdpScheduler&) = delete;
    UdpScheduler& operator=(const UdpScheduler&) = delete;

    bool send_now(const std::string& ip, std::uint16_t port,
                  const std::vector<std::uint8_t>& msg);

    Handle send_after(const std::string& ip, std::uint16_t port,
                      const std::vector<std::uint8_t>& msg, std::uint8_t delay_s);

    Handle send_periodic(const std::string& ip, std::uint16_t port,
                         const std::vector<std::uint8_t>& msg, std::uint8_t period_s);

    void cancel(Handle h);

private:
    enum class JobType { OneShot, Periodic };

    struct Job {
        Handle id{};
        JobType type{};
        std::string ip;
        std::uint16_t port{};
        std::vector<std::uint8_t> payload;
        std::chrono::seconds period{0};
        Clock::time_point due{};
    };

    struct Later {
        bool operator()(const Job& a, const Job& b) const { return a.due > b.due; }
    };

    std::thread worker_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop_;

    std::priority_queue<Job, std::vector<Job>, Later> pq_;
    std::unordered_set<Handle> cancelled_;
    std::atomic<Handle> next_id_;

    static void validate_range(std::uint8_t x);
    static bool make_sockaddr(const std::string& ip, std::uint16_t port, sockaddr_in& out);
    static bool udp_send(const std::string& ip, std::uint16_t port,
                         const std::vector<std::uint8_t>& msg);

    Handle schedule(JobType type, const std::string& ip, std::uint16_t port,
                    const std::vector<std::uint8_t>& msg, std::chrono::seconds period);

    void worker_loop();
};