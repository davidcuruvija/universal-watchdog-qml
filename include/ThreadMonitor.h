#pragma once
#include "watchable.h"
#include <string>
#include <chrono>
#include <mutex>

/**
 * @brief Monitors a thread's liveness via a periodic heartbeat mechanism.
 *
 * A thread is considered alive if it has called updateHeartbeat() within
 * the configured timeout window. Thread-safe for concurrent use.
 */
class ThreadMonitor : public Watchable {
public:
    /**
     * @brief Constructs a ThreadMonitor.
     * @param timeout Maximum allowed time between heartbeats.
     * @param threadName Human-readable name identifying the monitored thread.
     */
    ThreadMonitor(std::chrono::milliseconds timeout, std::string threadName);

    ThreadMonitor(const ThreadMonitor&) = delete;
    ThreadMonitor& operator =(const ThreadMonitor&) = delete;
    ThreadMonitor(ThreadMonitor&&) = delete;
    ThreadMonitor& operator =(ThreadMonitor&&) = delete;
    ~ThreadMonitor() override = default;

    /**
     * @brief Records the current time as the latest heartbeat.
     */
    void updateHeartbeat();

    /**
     * @brief Returns true if the last heartbeat was within the timeout window.
     */
    bool isAlive() const override;

    /**
     * @brief Returns the name of the monitored thread.
     */
    std::string name() const override;

    /**
     * @brief Returns the configured timeout duration.
     */
    std::chrono::milliseconds timeout() const {
        return m_timeout;
    }

private:
    const std::chrono::milliseconds m_timeout;
    const std::string m_threadName;
    std::chrono::steady_clock::time_point m_lastHeartbeat;
    mutable std::mutex m_mutex;
};