#include "watchdog.h"
#include "ProcessMonitor.h"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

static std::atomic<bool> g_running{true};

static void signalHandler(int) {
    g_running = false;
}

static void printUsage(const char* programName) {
    std::cerr << "Usage:   " << programName << " <interval_ms> <pid1> [pid2] ...\n"
              << "Example: " << programName << " 2000 $(pgrep -P 1 brave | head -1)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    int intervalMs = std::atoi(argv[1]);
    if (intervalMs <= 0) {
        std::cerr << "Error: interval must be a positive integer (ms).\n";
        return 1;
    }

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    Logger   logger(std::cout);
    Watchdog watchdog(logger);

    for (int i = 2; i < argc; ++i) {
        pid_t pid = static_cast<pid_t>(std::atoi(argv[i]));
        try {
            auto pm = std::make_shared<ProcessMonitor>(pid);
            watchdog.registerWatchable(pm);
            logger.log("Registered: " + pm->name() + " (PID " + std::to_string(pid) + ")");
        } catch (const std::invalid_argument& e) {
            std::cerr << "Warning: skipping PID " << pid << " — " << e.what() << "\n";
        }
    }

    if (watchdog.count() == 0) {
        std::cerr << "Error: no valid processes to monitor.\n";
        return 1;
    }

    logger.log("Watchdog started (interval: " + std::to_string(intervalMs) + "ms). Press Ctrl+C to stop.");
    watchdog.start(intervalMs);

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    watchdog.stop();
    logger.log("Watchdog stopped.");
    return 0;
}

