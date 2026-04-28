#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "watchable.h"

class Watchdog{
public:
    Watchdog();
    ~Watchdog();

    // Non-copyable, non-movable — owns a thread
    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;
    Watchdog(Watchdog&&) = delete;
    Watchdog& operator=(Watchdog&&) = delete;

    // Adds a watchable object to be monitored.
    void registerWatchable(std::shared_ptr<Watchable> w);

    // Starts the monitoring loop in a background thread.
    // Each iteration sleeps for interval_ms milliseconds.
    void start(int interval_ms);

    // Signals the monitoring loop to stop and joins the thread.
    void stop();

private:
    void monitorLoop(int interval_ms);

    std::vector<std::shared_ptr<Watchable>> m_watchables;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_running;
    std::thread m_monitorThread;

};