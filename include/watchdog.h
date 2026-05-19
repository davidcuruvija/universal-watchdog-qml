#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <unordered_map>

#include "watchable.h"
#include "Manageable.h"
#include "logger.h"
#include "Config.h"

/**
 * @class Watchdog
 * @brief Periodically monitors registered Watchable objects and reports their status.
 *
 * The Watchdog runs a background thread that iterates over all registered
 * Watchable objects at the configured poll interval. To keep the log
 * informative without spamming, status is reported in two cases:
 *
 *   1. When an object's alive state changes from the previous check.
 *   2. Periodically (every Config::summaryInterval), a full summary of
 *      all monitored objects is logged regardless of state.
 *
 * The class is non-copyable and non-movable because it owns a background thread.
 *
 */

enum class SortMode { MEMORY, CPU, NAME };

class Watchdog{
public:
    /**
     * @brief Construct a Watchdog with the given logger and configuration.
     * @param logger Reference to a Logger instance. Must outlive the Watchdog.
     * @param config Configuration values for intervals. Defaults are used
     *               if not specified.
     */
    explicit Watchdog(Logger& logger, Config config = Config{});

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
     * @return true if registered successfully, false if a duplicate name exists.
     */
    bool registerWatchable(std::shared_ptr<Watchable> w);

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
     * @brief Starts the monitoring loop in a background thread using the configured poll interval.
     *
     */
    void start();

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

    /**
     * @brief Returns a snapshot of all monitored entities with their current status.
     * Thread-safe. Takes the lock, copies data, and returns.
     * Results are sorted according to the current SortMode (default: MEMORY descending).
     * @return Vector of WatchableStatus structs.
     */
    std::vector<WatchableStatus> getStatusSnapshot() const;

    /**
     * @brief Sets the field used to sort the status snapshot.
     * Thread-safe.
     */
    void setSortMode(SortMode mode);

    /**
     * @brief Send SIGTERM to a monitored process by its display name.
     *
     * Thread-safe. Returns false if no matching Manageable is found.
     */
    bool killByName(const std::string& name);

    /**
     * @brief Kill and relaunch a monitored process by its display name.
     *
     * Thread-safe. After a successful restart the tracked PID is updated.
     * Returns false if no matching Manageable is found or restart fails.
     */
    bool restartByName(const std::string& name);

    /**
     * @brief Returns a detailed snapshot including granular state for ProcessMonitors.
     * @return Vector of (name, statusString) pairs.
     */
    std::vector<std::pair<std::string, std::string>> getDetailedSnapshot() const;

private:
    
    void monitorLoop();
    void checkAndLog();

    Logger& m_logger;
    Config m_config;
    SortMode m_sortMode{SortMode::MEMORY};
    std::vector<std::shared_ptr<Watchable>> m_watchables;
    std::unordered_map<std::string, std::string> m_lastStatus;
    std::chrono::steady_clock::time_point m_lastSummary;

    mutable std::mutex m_mutex;
    std::atomic<bool> m_running;
    std::thread m_monitorThread;

    std::condition_variable m_stopCv;
    std::mutex m_stopMutex;
};