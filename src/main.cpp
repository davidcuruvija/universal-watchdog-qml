#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "../include/watchdog.h"
#include "../include/logger.h"
#include "../include/ProcessMonitor.h"
#include "../include/ThreadMonitor.h"
#include "../include/Config.h"

namespace {

// Global flag set by the SIGINT handler. Atomic so it's safe to read from
// the signal handler context.
std::atomic<bool> g_shutdownRequested{false};

void handleSignal(int /*signum*/) {
    g_shutdownRequested.store(true);
}

void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " <pid> [<pid> ...]\n"
              << "\n"
              << "Monitors the given process IDs and logs ALIVE/DEAD status\n"
              << "every second. Press Ctrl+C to stop.\n"
              << "\n"
              << "Example:\n"
              << "  " << progName << " 1 1234 5678\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    Config config;
    Logger logger(config.logFile);

    Watchdog watchdog(logger, config);

    std::cout << "Watchdog starting. Monitoring PIDs:";

    // Register a ProcessMonitor for each PID passed on the command line
    for (int i = 1; i < argc; ++i) {
        try {
            const pid_t pid = static_cast<pid_t>(std::stoi(argv[i]));
            std::cout << " " << pid;
            watchdog.registerWatchable(std::make_shared<ProcessMonitor>(pid));
        } catch (const std::exception& ex) {
            std::cerr << "\nWarning: could not parse '" << argv[i]
                      << "' as a PID: " << ex.what() << "\n";
        }
    }
    std::cout << "\n";
    std::cout << "Logging to console and to 'watchdog.log'.\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    // Start monitoring with a 1-second interval
    watchdog.start();

    // Idle main thread until shutdown is requested
    while (!g_shutdownRequested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nShutdown requested, stopping watchdog...\n";
    watchdog.stop();
    std::cout << "Goodbye.\n";

    return EXIT_SUCCESS;
}