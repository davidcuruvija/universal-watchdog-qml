#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include "../../include/watchdog.h"
#include "../../include/logger.h"
#include "../../include/Config.h"
#include "../../include/ThreadMonitor.h"
#include "../../include/TerminalRenderer.h"

using namespace std::chrono_literals;

int main() {
    Config config;
    config.heartbeatTimeout = 2000ms; // 2 seconds timeout for clear demonstration
    Logger logger("demo_threads.log");
    Watchdog watchdog(logger, config);
    TerminalRenderer renderer;
    
    // Create monitors for our 4 distinct thread scenarios
    auto goodMonitor = std::make_shared<ThreadMonitor>(config, "Database_Writer");
    auto stuckMonitor = std::make_shared<ThreadMonitor>(config, "Image_Processor");
    auto zombieMonitor = std::make_shared<ThreadMonitor>(config, "Network_Downloader");
    auto deadMonitor = std::make_shared<ThreadMonitor>(config, "Backup_Service");

    // Register them into the watchdog
    watchdog.registerWatchable(goodMonitor);
    watchdog.registerWatchable(stuckMonitor);
    watchdog.registerWatchable(zombieMonitor);
    watchdog.registerWatchable(deadMonitor);

    watchdog.start(); // Starts background logger

    std::atomic<bool> run{true};

    // 1. Database Writer (ALIVE)
    // Behaves perfectly: infinite loop, doing work, sends heartbeat every 0.5s.
    std::thread t1([&]() {
        while (run) {
            std::this_thread::sleep_for(500ms);
            goodMonitor->updateHeartbeat();
        }
    });

    // 2. Image Processor (STUCK)
    // Works normally for 5s, then enters a 'deadlock' (e.g. infinite math).
    // Lost from heartbeat but OS reports the thread still exists.
    std::thread t2([&]() {
        for (int i = 0; i < 10; ++i) { // 5 seconds of healthy behavior
            std::this_thread::sleep_for(500ms);
            stuckMonitor->updateHeartbeat();
        }
        // DEADLOCK! (Thread is stuck, OS sees it, Watchdog reports STUCK)
        while(run) {
            std::this_thread::sleep_for(100ms); // Simulate CPU heavy / blocked operation
        }
    });

    // 3. Network Downloader (ZOMBIE)
    // Works normally for 3s then unexpectedly crashes/exits.
    // Since Watchdog still tracks it, it will detect that the OS thread is gone.
    std::thread t3([&]() {
        for (int i = 0; i < 6; ++i) { // 3 seconds of healthy behavior
            std::this_thread::sleep_for(500ms);
            zombieMonitor->updateHeartbeat();
        }
        // Thread terminates naturally. OS reaps it.
    });
    t3.detach(); // Detach to let it exit cleanly.

    // 4. Backup Service (DEAD)
    // A service that failed to start (e.g. failed connection immediately on start).
    // No OS thread exists, no heartbeat ever received. Stays DEAD from the beginning.

    renderer.clearAll();
    
    // Run simulation for 15s and refresh the UI
    for (int i = 0; i < 15; ++i) {
        renderer.clearVisible();
        std::cout << "\n\033[36m\033[1m  === EMBEDDED WATCHDOG SIMULATION ===\033[0m\n";
        std::cout << "  This program simulates how an application uses Watchdog internally.\n";
        
        auto snapshot = watchdog.getDetailedSnapshot();
        renderer.printWatchHeader("Simulation running...", true);
        renderer.printList(snapshot);
        
        std::cout << "\n  Remaining simulation time: " << 15 - i << "s\n";
        
        // Narrative explanations printed at the bottom:
        std::cout << "\033[2m"; // Dim text
        if (i < 3) std::cout << "  [INFO] All threads behaving normally (except Backup_Service which never started).\n";
        else if (i < 6) std::cout << "  [INFO] Network_Downloader just crashed and disappeared from the OS! (Waiting for timeout...)\n";
        else if (i < 8) std::cout << "  [INFO] Image_Processor enters deadlock! Work loop blocked.\n";
        else std::cout << "  [INFO] Watchdog successfully detected all problematic states!\n";
        std::cout << "\033[0m";

        std::this_thread::sleep_for(1s);
    }

    // Cleanup
    run = false;
    t1.join();
    t2.join();
    watchdog.stop();
    return 0;
}
