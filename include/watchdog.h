#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>

#include "watchable.h"
#include "logger.h"
#include "Config.h"

/**
 * @class Watchdog
 * @brief Periodically monitors registered Watchable objects and reports their status.
 *
 * The Watchdog runs a background thread that iterates over all registered
 * Watchable objects at a fixed interval, querying their `isAlive()` state and
 * logging the result. It does not take any corrective action when an object is
 * reported as dead — it only detects and reports.
 *
 * The class is non-copyable and non-movable because it owns a background thread.
 *
 */

class Watchdog{
public:
    /**
     * @brief Constructs a Watchdog that logs status updates via the given logger.
     */
    explicit Watchdog(Logger& logger);

    /**
     * @brief Destructor. Ensures the monitoring thread is stopped cleanly.
     */
    ~Watchdog();

    // Non-copyable, non-movable — owns a thread
    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;
    Watchdog(Watchdog&&) = delete;
    Watchdog& operator=(Watchdog&&) = delete;

    /**
     * @brief Adds a Watchable object to be monitored.
     *
     * Thread-safe — can be called while the monitoring loop is running.
     *
     */
    void registerWatchable(std::shared_ptr<Watchable> w);

    /**
     * @brief Remove a registered Watchable by its reported name.
     *
     * Thread-safe. If multiple watchables share the same name, only the
     * first one found is removed.
     *
     * @return true if a matching object was found and removed; false otherwise.
     */
    bool unregisterByName(const std::string& name);

    /**
     * @brief Starts the monitoring loop using project-wide config.
     * @param config Project configuration. Uses config.pollInterval.
     */
    void start(const Config& config);

    /**
     * @brief Starts the monitoring loop in a background thread.
     *
     * Each iteration queries all registered Watchable objects and logs their
     * status. The loop sleeps for `interval_ms` milliseconds between iterations.
     *
     * Calling start() while already running is a no-op.
     *
     */
    void start(int interval_ms);

    /**
     * @brief Signals the monitoring thread to stop and joins it.
     *
     * Safe to call when the monitor is not running. Returns quickly thanks
     * to a condition variable that wakes the monitoring thread immediately.
     * 
     */
    void stop();

    /**
     * @brief Checks whether the monitoring loop is currently active.
     * @return true if start() has been called and stop() has not yet returned.
     */
    bool isRunning() const;

    /**
     * @brief Returns the number of registered Watchable objects.
     * @return Count of registered objects.
     */
    std::size_t count() const;

private:
    /**
     * @brief The body of the monitoring thread.
     * @param interval_ms Sleep duration between iterations.
     */
    void monitorLoop(int interval_ms);

    Logger& m_logger;
    std::vector<std::shared_ptr<Watchable>> m_watchables;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_running;
    std::thread m_monitorThread;

    std::condition_variable m_stopCv;
    std::mutex m_stopMutex;
};