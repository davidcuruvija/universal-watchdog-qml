#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
#include "../include/Config.h"
#include "../include/ProcessMonitor.h"
#include "../include/ThreadMonitor.h"
#include "../include/logger.h"
#include "../include/watchdog.h"

using namespace std::chrono_literals;

static void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

int main() {
    Config config;
    config.pollInterval = 500ms;
    config.heartbeatTimeout = 1000ms;
    config.summaryInterval = 60000ms;

    Logger logger(std::cout);
    Watchdog watchdog(logger, config);

    std::atomic<bool> running{true};

    section("Scenario 1: healthy thread (worker-healthy)");
    auto healthy = std::make_shared<ThreadMonitor>(config, "worker-healthy");
    watchdog.registerWatchable(healthy);

    std::thread tHealthy([&] {
        while (running.load()) {
            healthy->updateHeartbeat();
            std::this_thread::sleep_for(300ms);
        }
    });

    section("Scenario 2: short-timeout thread (worker-sensitive, timeout=400ms)");
    auto sensitive = std::make_shared<ThreadMonitor>(400ms, "worker-sensitive");
    watchdog.registerWatchable(sensitive);

    std::thread tSensitive([&] {
        const auto deadline = std::chrono::steady_clock::now() + 4s;
        while (running.load() && std::chrono::steady_clock::now() < deadline) {
            sensitive->updateHeartbeat();
            std::this_thread::sleep_for(200ms);
        }
        std::cout << "\n[demo] worker-sensitive: heartbeat stopped — short timeout means faster DEAD detection.\n";
        while (running.load()) {
            std::this_thread::sleep_for(500ms);
        }
    });

    section("Scenario 3: thread that stops heartbeat after 3s (worker-stopping)");
    auto stopping = std::make_shared<ThreadMonitor>(config, "worker-stopping");
    watchdog.registerWatchable(stopping);

    std::thread tStopping([&] {
        const auto deadline = std::chrono::steady_clock::now() + 3s;
        while (running.load() && std::chrono::steady_clock::now() < deadline) {
            stopping->updateHeartbeat();
            std::this_thread::sleep_for(300ms);
        }
        std::cout << "\n[demo] worker-stopping: heartbeat stopped — watchdog should detect DEAD.\n";
        while (running.load()) {
            std::this_thread::sleep_for(500ms);
        }
    });

    section("Scenario 4: thread that exits abruptly after 5s (worker-crash)");
    auto crashing = std::make_shared<ThreadMonitor>(config, "worker-crash");
    watchdog.registerWatchable(crashing);

    std::thread tCrash([&] {
        const auto deadline = std::chrono::steady_clock::now() + 5s;
        while (std::chrono::steady_clock::now() < deadline) {
            crashing->updateHeartbeat();
            std::this_thread::sleep_for(300ms);
        }
        std::cout << "\n[demo] worker-crash: thread exiting abruptly — OS TID check should detect DEAD immediately.\n";
    });

    section("Scenario 5: child process that exits after 4s (ProcessMonitor)");
    pid_t child = fork();
    if (child == 0) {
        std::this_thread::sleep_for(4s);
        std::exit(0);
    }
    std::cout << "[demo] Monitoring child PID: " << child << "\n";
    auto childProcess = std::make_shared<ProcessMonitor>(child);
    watchdog.registerWatchable(childProcess);

    std::cout << "\n[demo] Starting watchdog. Running for 10 seconds...\n\n";
    watchdog.start();

    std::this_thread::sleep_for(10s);

    std::cout << "\n[demo] Shutting down.\n";
    running.store(false);
    watchdog.stop();

    tHealthy.join();
    tSensitive.join();
    tStopping.join();
    if (tCrash.joinable()) tCrash.join();

    waitpid(child, nullptr, 0);

    return 0;
}
