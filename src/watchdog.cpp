#include "../include/watchdog.h"

#include <chrono>
#include <iostream>

Watchdog::Watchdog(Logger& logger)
    : m_logger(logger)
    , m_running(false) {
}

Watchdog::~Watchdog() {
    stop();  // Safety: ensure thread doesn't outlive the object
}

void Watchdog::registerWatchable(std::shared_ptr<Watchable> w) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchables.push_back(std::move(w));
}

void Watchdog::start(int interval_ms) {
    if (m_running.exchange(true)) {
        return;  // Already running
    }
    m_monitorThread = std::thread(&Watchdog::monitorLoop, this, interval_ms);
}

void Watchdog::stop() {
    if (!m_running.exchange(false)) {
        return;  // Was not running
    }
    if (m_monitorThread.joinable()) {
        m_monitorThread.join();
    }
}

bool Watchdog::isRunning() const {
    return m_running.load();
}

std::size_t Watchdog::count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_watchables.size();
}

// TODO: Replace with Logger
void Watchdog::monitorLoop(int interval_ms) {
    while (m_running.load()) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& w : m_watchables) {
                const std::string status = w->isAlive() ? "ALIVE" : "DEAD";
                m_logger.log("[" + w->name() + "] " + status);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
}
