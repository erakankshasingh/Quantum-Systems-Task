#include "cpp_udpScheduler_asingh.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

UdpScheduler::UdpScheduler() : stop_(false), next_id_(1) {
    worker_ = std::thread([this] { worker_loop(); });
}

UdpScheduler::~UdpScheduler() {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        stop_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
}

bool UdpScheduler::send_now(const std::string& ip, std::uint16_t port,
                            const std::vector<std::uint8_t>& msg)
{
    return udp_send(ip, port, msg);
}

UdpScheduler::Handle UdpScheduler::send_after(const std::string& ip, std::uint16_t port,
                                              const std::vector<std::uint8_t>& msg,
                                              std::uint8_t delay_s)
{
    validate_range(delay_s);
    return schedule(JobType::OneShot, ip, port, msg, std::chrono::seconds(delay_s));
}

UdpScheduler::Handle UdpScheduler::send_periodic(const std::string& ip, std::uint16_t port,
                                                 const std::vector<std::uint8_t>& msg,
                                                 std::uint8_t period_s)
{
    validate_range(period_s);
    return schedule(JobType::Periodic, ip, port, msg, std::chrono::seconds(period_s));
}

void UdpScheduler::cancel(Handle h) {
    std::lock_guard<std::mutex> lk(mtx_);
    cancelled_.insert(h);
    cv_.notify_all();
}

void UdpScheduler::validate_range(std::uint8_t x) {
    if (x < 1 || x > 255) {
        throw std::invalid_argument("X must be in [1,255] seconds");
    }
}

bool UdpScheduler::make_sockaddr(const std::string& ip, std::uint16_t port, sockaddr_in& out) {
    std::memset(&out, 0, sizeof(out));
    out.sin_family = AF_INET;
    out.sin_port   = htons(port);
    return inet_pton(AF_INET, ip.c_str(), &out.sin_addr) == 1;
}

bool UdpScheduler::udp_send(const std::string& ip, std::uint16_t port,
                            const std::vector<std::uint8_t>& msg)
{
    sockaddr_in addr{};
    if (!make_sockaddr(ip, port, addr)) {
        throw std::invalid_argument("Invalid IPv4 address: " + ip);
    }

    const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;

    const std::uint8_t* data = msg.empty() ? nullptr : msg.data();
    const std::size_t len = msg.size();

    if (len > 0 && data == nullptr) {
        ::close(fd);
        throw std::invalid_argument("Payload pointer is null");
    }

    const ssize_t sent = ::sendto(fd, data, len, 0,
                                  reinterpret_cast<const sockaddr*>(&addr),
                                  sizeof(addr));
    ::close(fd);
    return sent == static_cast<ssize_t>(len);
}

UdpScheduler::Handle UdpScheduler::schedule(JobType type, const std::string& ip,
                                            std::uint16_t port,
                                            const std::vector<std::uint8_t>& msg,
                                            std::chrono::seconds period)
{
    sockaddr_in tmp{};
    if (!make_sockaddr(ip, port, tmp)) {
        throw std::invalid_argument("Invalid IPv4 address: " + ip);
    }

    Job j;
    j.id = next_id_.fetch_add(1, std::memory_order_relaxed);
    j.type = type;
    j.ip = ip;
    j.port = port;
    j.payload = msg;
    j.period = period;
    j.due = Clock::now() + period;

    {
        std::lock_guard<std::mutex> lk(mtx_);
        pq_.push(std::move(j));
    }
    cv_.notify_all();
    return j.id;
}

void UdpScheduler::worker_loop() {
    std::unique_lock<std::mutex> lk(mtx_);

    while (!stop_) {
        while (!pq_.empty() && cancelled_.count(pq_.top().id)) {
            pq_.pop();
        }

        if (pq_.empty()) {
            cv_.wait(lk, [this] { return stop_ || !pq_.empty(); });
            continue;
        }

        const auto due = pq_.top().due;
        cv_.wait_until(lk, due, [this] { return stop_; });
        if (stop_) break;

        while (!pq_.empty() && cancelled_.count(pq_.top().id)) {
            pq_.pop();
        }
        if (pq_.empty()) continue;

        if (Clock::now() < pq_.top().due) continue;

        Job job = pq_.top();
        pq_.pop();

        if (cancelled_.count(job.id)) continue;

        lk.unlock();
        (void)udp_send(job.ip, job.port, job.payload);
        lk.lock();

        if (job.type == JobType::Periodic && !cancelled_.count(job.id)) {
            job.due = Clock::now() + job.period;
            pq_.push(std::move(job));
        }
    }
}