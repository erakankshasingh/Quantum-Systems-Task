# Quantum Systems Task

This repository contains my solutions for the Quantum Systems Linux/C++/Python assignment, including bug fixing, analysis notes, and a UDP scheduler implementation.

---

## Repository Structure

### C++ Bug Analysis
- `cpp_task.cpp` — original C++ code for the bug analysis task
- `cpp_task_fix_asingh.cpp` — corrected C++ implementation
- `txt_cpp_bug_analysis_asingh.txt` — explanation of the identified issues and fixes

### UDP Scheduler
- `cpp_udpScheduler_asingh.hpp` — class declaration for the UDP scheduler
- `cpp_udpScheduler_asingh.cpp` — implementation of the UDP scheduler
- `cpp_main_asingh.cpp` — demo application using the scheduler
- `txt_udpscheduler_interface_analysis_asingh.txt` — design explanation

### Python Bug Analysis
- `python_task.py` — original Python matrix rotation code with bug
- `python_task_fix_asingh.py` — corrected Python implementation
- `txt_python_bug_analysis_asingh.txt` — explanation of the bug and fix

---

## UDP Scheduler

### Overview
This solution implements a C++17 class `UdpScheduler` that provides an interface for:

1. Sending a UDP message immediately to a specific IPv4 address and port
2. Scheduling a one-shot UDP message after a delay of `X` seconds
3. Scheduling periodic UDP sending every `X` seconds

The delay/period range is validated to be within **1 to 255 seconds**, as required.

### Implementation Details
The implementation uses:

- POSIX UDP sockets (`socket`, `sendto`)
- One background worker thread
- A priority queue to manage scheduled jobs
- `std::chrono::steady_clock` for robust timeout and scheduling behavior

### Files
- `cpp_udpScheduler_asingh.hpp` — class declaration / interface
- `cpp_udpScheduler_asingh.cpp` — implementation
- `cpp_main_asingh.cpp` — demo application

---

## Build

Compile with `g++` and `pthread` support:

```bash
g++ -std=c++17 cpp_udpScheduler_asingh.cpp cpp_main_asingh.cpp -o udp_demo -pthread
```

## Run

```bash
./udp_demo
```

---

## Demo Behavior

The demo application in `cpp_main_asingh.cpp` performs the following actions:

- Sends the message `"hello"` immediately to `127.0.0.1:9000`
- Schedules a one-shot send of the same message after 3 seconds
- Starts periodic sending of the same message every 2 seconds
- Waits for 7 seconds
- Cancels the periodic job
- Exits cleanly

## Notes

- The implementation uses a composition-friendly design: `UdpScheduler` can be used directly or as a member inside another class.
- Periodic sending starts after the first period interval, not immediately.
- `cancel()` is provided to stop scheduled periodic jobs.
- IPv4 addresses are validated using `inet_pton`.
- The message payload is stored internally as `std::vector<std::uint8_t>`, which supports both text and binary data.

## Example Usage

```cpp
UdpScheduler scheduler;
std::vector<std::uint8_t> msg{'h', 'e', 'l', 'l', 'o'};

scheduler.send_now("127.0.0.1", 9000, msg);

auto one_shot = scheduler.send_after("127.0.0.1", 9000, msg, 3);
auto periodic = scheduler.send_periodic("127.0.0.1", 9000, msg, 2);

// later
scheduler.cancel(periodic);
```