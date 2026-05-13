#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <ctime>
#include <iomanip>
#include <map>
#include <set>

#include "../include/watchdog.h"
#include "../include/logger.h"
#include "../include/ProcessMonitor.h"
#include "../include/ThreadMonitor.h"
#include "../include/Config.h"
#include "../include/CommandParser.h"
#include "../include/TerminalRenderer.h"

namespace {

// Global flag set by the SIGINT handler. Atomic so it's safe to read from
// the signal handler context.
std::atomic<bool> g_shutdownRequested{false};

void handleSignal(int /*signum*/) {
    g_shutdownRequested.store(true);
}

}  // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    Config config;
    Logger logger(config.logFile);
    Watchdog watchdog(logger, config);
    CommandParser parser;
    TerminalRenderer renderer;

    // Backward compatibility: parse PIDs from command line
    for (int i = 1; i < argc; ++i) {
        try {
            const pid_t pid = static_cast<pid_t>(std::stoi(argv[i]));
            auto pm = std::make_shared<ProcessMonitor>(pid);
            watchdog.registerWatchable(pm);
        } catch (const std::exception& ex) {
            std::cerr << "Warning: could not parse '" << argv[i]
                      << "' as a PID: " << ex.what() << "\n";
        }
    }

    renderer.printBanner();
    watchdog.start();

    // Print initial list if any were passed via CLI
    if (watchdog.count() > 0) {
        renderer.printList(watchdog.getStatusSnapshot());
    }

    std::string line;
    while (!g_shutdownRequested.load()) {
        renderer.printPrompt();
        
        if (!std::getline(std::cin, line)) {
            break;  // EOF (Ctrl+D)
        }
        if (line.empty()) continue;

        ParsedCommand cmd = parser.parse(line);

        switch (cmd.type) {
            case ParsedCommand::Type::ADD_PROCESS: {
                try {
                    pid_t pid = std::stoi(cmd.args[0]);
                    auto pm = std::make_shared<ProcessMonitor>(pid);
                    watchdog.registerWatchable(pm);
                    renderer.printSuccess("Now monitoring: " + pm->name());
                } catch(const std::exception& ex) {
                    renderer.printError("Failed to add process: " + std::string(ex.what()));
                }
                break;
            }
            case ParsedCommand::Type::ADD_THREAD: {
                auto timeout = config.heartbeatTimeout;
                if (cmd.args.size() > 1) {
                    timeout = std::chrono::milliseconds(std::stoi(cmd.args[1]));
                }
                auto tm = std::make_shared<ThreadMonitor>(timeout, cmd.args[0]);
                watchdog.registerWatchable(tm);
                renderer.printSuccess("Now monitoring thread: " + cmd.args[0]);
                break;
            }
            case ParsedCommand::Type::REMOVE: {
                std::string name = cmd.args[0];
                if (watchdog.unregisterByName(name)) {
                    renderer.printSuccess("Removed: " + name);
                } else {
                    renderer.printWarning("Not found: " + name);
                }
                break;
            }
            case ParsedCommand::Type::LIST:
                renderer.printList(watchdog.getStatusSnapshot());
                break;
            case ParsedCommand::Type::WATCH: {
                std::map<std::string, bool> previousState;
                bool firstIteration = true;

                while (!g_shutdownRequested.load()) {
                    auto currentSnapshot = watchdog.getStatusSnapshot();
                    std::set<std::string> changedEntities;
                    
                    if (!firstIteration) {
                        for (const auto& entry : currentSnapshot) {
                            auto it = previousState.find(entry.first);
                            if (it != previousState.end() && it->second != entry.second) {
                                changedEntities.insert(entry.first);
                            }
                        }
                    }
                    
                    previousState.clear();
                    for (const auto& entry : currentSnapshot) {
                        previousState[entry.first] = entry.second;
                    }
                    firstIteration = false;

                    renderer.clearVisible();
                    
                    auto now = std::chrono::system_clock::now();
                    std::time_t t = std::chrono::system_clock::to_time_t(now);
                    std::tm tm{};
                    localtime_r(&t, &tm);
                    char buf[30];
                    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
                    
                    renderer.printWatchHeader(buf);
                    renderer.printList(currentSnapshot, changedEntities);

                    bool stop = false;
                    // Wait up to 1 second, but check for stdin every 100ms
                    for (int i = 0; i < 10; ++i) {
                        struct pollfd fds;
                        fds.fd = STDIN_FILENO;
                        fds.events = POLLIN;
                        if (poll(&fds, 1, 0) == 1) {
                            stop = true;
                            break;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    if (stop) {
                        std::string dummy;
                        std::getline(std::cin, dummy); // Consume the ENTER
                        renderer.clearVisible();
                        renderer.printBanner();
                        break;
                    }
                }
                break;
            }
            case ParsedCommand::Type::CONFIG:
                renderer.printConfig(config.logFile, 
                                     config.heartbeatTimeout.count(), 
                                     config.pollInterval.count(), 
                                     config.summaryInterval.count());
                break;
            case ParsedCommand::Type::CLEAR:
                renderer.clearAll();
                renderer.printBanner();
                break;
            case ParsedCommand::Type::HELP:
                renderer.printHelp();
                break;
            case ParsedCommand::Type::QUIT:
                g_shutdownRequested.store(true);
                break;
            case ParsedCommand::Type::UNKNOWN:
                renderer.printError(cmd.errorMessage);
                break;
        }
    }

    renderer.printInfo("Shutting down watchdog...");
    watchdog.stop();
    renderer.printInfo("Goodbye. \xf0\x9f\x90\x95"); // 🐕

    return EXIT_SUCCESS;
}