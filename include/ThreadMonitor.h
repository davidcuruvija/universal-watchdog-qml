#pragma once
#include "watchable.h"
#include "Config.h"
#include <string>
#include <chrono>
#include <mutex>
#include <optional>
#include <sys/types.h>

#include <sys/types.h>

/**
 * @brief Granular thread states based on heartbeat and existence.
 */
enum class ThreadState {
    ALIVE,    ///< Heartbeat is current
    STUCK,    ///< Heartbeat timed out, but thread still exists in /proc/self/task/
    ZOMBIE,   ///< Heartbeat timed out, thread doesn't exist, but monitor wasn't cleaned up
    DEAD      ///< Never received a heartbeat or completely dead
};

/**
 * @brief Monitors a thread's liveness via a periodic heartbeat mechanism.
 *
 * A thread is considered alive if it has called updateHeartbeat() within
 * the configured timeout window. Thread-safe for concurrent use.
 */
class ThreadMonitor : public Watchable {
public:
    /**
     * @brief Constructs a ThreadMonitor using project-wide config.
     * @param config Project configuration. Uses config.heartbeatTimeout.
     * @param threadName Human-readable name identifying the monitored thread.
     * 
     * @note The caller is responsible for ensuring threadName is unique
     *       within a single Watchdog instance. See the other constructor
     *       for naming guidance.
     * 
     */
    ThreadMonitor(const Config& config, std::string threadName);

    /**
     * @brief Constructs a ThreadMonitor.
     * @param timeout Maximum allowed time between heartbeats.
     * @param threadName Human-readable name identifying the monitored thread.
     * 
     * @note The caller is responsible for ensuring threadName is unique
     *       within a single Watchdog instance. Watchdog uses the name to
     *       track per-watchable state and identify entries in
     *       unregisterByName(). Duplicate names cause cache collisions
     *       and ambiguous removal.
     *
     *       Suggested naming patterns:
     *         - Role + instance index: "worker-1", "worker-2"
     *         - Role + counter: "worker#0", "worker#1"
     *         - Role + purpose: "db-reader", "db-writer"
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
     * @brief Returns the detailed state of the thread based on heartbeat and kernel existence.
     * @return ThreadState indicating ALIVE, STUCK, ZOMBIE, or DEAD.
     */
    ThreadState getThreadState() const;

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
    std::optional<pid_t> m_nativeTid;
    mutable std::mutex m_mutex;
};