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
#include <sys/ioctl.h>

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
            if (!watchdog.registerWatchable(pm)) {
                std::cerr << "Warning: PID " << pid << " is already being monitored.\n";
            }
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
                    if (watchdog.registerWatchable(pm)) {
                        renderer.printSuccess("Now monitoring: " + pm->name());
                    } else {
                        renderer.printWarning("Already monitoring: " + pm->name());
                    }
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
                if (watchdog.registerWatchable(tm)) {
                    renderer.printSuccess("Now monitoring thread: " + cmd.args[0]);
                } else {
                    renderer.printWarning("Already monitoring: " + cmd.args[0]);
                }
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
            case ParsedCommand::Type::KILL: {
                std::string name = cmd.args[0];
                if (watchdog.killByName(name)) {
                    renderer.printSuccess("SIGTERM sent to: " + name);
                } else {
                    renderer.printWarning("Not found or not killable: " + name);
                }
                break;
            }
            case ParsedCommand::Type::RESTART: {
                std::string name = cmd.args[0];
                renderer.printInfo("Restarting: " + name + " ...");
                if (watchdog.restartByName(name)) {
                    renderer.printSuccess("Restarted: " + name);
                } else {
                    renderer.printWarning("Not found, not restartable, or restart failed: " + name);
                }
                break;
            }
            case ParsedCommand::Type::LIST:
                renderer.printList(watchdog.getStatusSnapshot());
                break;
            case ParsedCommand::Type::SORT: {
                const std::string& field = cmd.args[0];
                SortMode mode = SortMode::MEMORY;
                if (field == "cpu")    mode = SortMode::CPU;
                else if (field == "name") mode = SortMode::NAME;
                watchdog.setSortMode(mode);
                renderer.printSuccess("Sort order set to: " + field);
                break;
            }
            case ParsedCommand::Type::WATCH: {
                std::map<std::string, bool> previousState;
                std::vector<WatchableStatus> lastSnapshot;
                std::string lastTimeStr;
                bool firstIteration = true;
                int lastLiveLines = 0;

                // Clear screen once when entering watch mode
                renderer.clearVisible();

                while (!g_shutdownRequested.load()) {
                    auto currentSnapshot = watchdog.getStatusSnapshot();
                    std::set<std::string> changedEntities;

                    if (!firstIteration) {
                        for (const auto& entry : currentSnapshot) {
                            auto it = previousState.find(entry.name);
                            if (it != previousState.end() && it->second != entry.alive) {
                                changedEntities.insert(entry.name);
                            }
                        }
                    }

                    previousState.clear();
                    for (const auto& entry : currentSnapshot) {
                        previousState[entry.name] = entry.alive;
                    }

                    bool hasChanges = !changedEntities.empty();

                    if (hasChanges) {
                        // We have changes! First, rewrite the old Live View as a frozen Snapshot
                        if (lastLiveLines > 0) {
                            renderer.moveCursorUp(lastLiveLines);
                            renderer.clearFromCursor();
                            renderer.printWatchHeader(lastTimeStr, false);
                            renderer.printList(lastSnapshot);
                        }

                        // Now print the new state WITH change indicators as a second frozen Snapshot
                        auto now = std::chrono::system_clock::now();
                        std::time_t t = std::chrono::system_clock::to_time_t(now);
                        std::tm tm{};
                        localtime_r(&t, &tm);
                        char buf[30];
                        std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
                        renderer.printWatchHeader(buf, false);
                        renderer.printList(currentSnapshot, changedEntities);
                        std::cout << std::flush;
                    } else if (!firstIteration) {
                        // No changes: check if table fits in terminal
                        int liveLines = 10 + (int)currentSnapshot.size();
                        struct winsize ws;
                        int termRows = 24; // safe default
                        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
                            termRows = ws.ws_row;
                        }
                        
                        if (liveLines < termRows && lastLiveLines > 0) {
                            // Table fits: move cursor up and overwrite in-place
                            renderer.moveCursorUp(lastLiveLines);
                            renderer.clearFromCursor();
                        } else {
                            // Table too tall: fall back to full screen clear
                            renderer.clearVisible();
                        }
                    }

                    // Get current time
                    auto now = std::chrono::system_clock::now();
                    std::time_t t = std::chrono::system_clock::to_time_t(now);
                    std::tm tm{};
                    localtime_r(&t, &tm);
                    char buf[30];
                    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);

                    // Print the live table
                    renderer.printWatchHeader(buf);
                    renderer.printList(currentSnapshot);
                    std::cout << std::flush;

                    // Calculate how many lines the live section occupies:
                    // printWatchHeader = 3 lines (\n + LIVE VIEW line + Press ENTER line)
                    // printList = 7 + N lines (\n + top + header + sep + N entries + bottom + summary + \n)
                    lastLiveLines = 10 + (int)currentSnapshot.size();
                    lastSnapshot = currentSnapshot;
                    lastTimeStr = buf;

                    firstIteration = false;

                    struct pollfd fds;
                    fds.fd = STDIN_FILENO;
                    fds.events = POLLIN;
                    if (poll(&fds, 1, config.pollInterval.count()) > 0) {
                        std::string dummy;
                        std::getline(std::cin, dummy);
                        std::cout << std::endl;
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