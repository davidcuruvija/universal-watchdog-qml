#pragma once
#include <string>

/**
 * @brief Abstract interface for all monitored objects.
 *
 * Any component that can be monitored by the watchdog must implement
 * this interface.
 */
class Watchable {
public:
    virtual ~Watchable() = default;

    /**
     * @brief Returns true if the monitored object is considered alive.
     */
    virtual bool isAlive() const = 0;

    /**
     * @brief Returns the human-readable name of the monitored object.
     */
    virtual std::string name() const = 0;

    /**
     * @brief Returns resident memory usage in kilobytes, or 0 if not applicable.
     */
    virtual long memoryKB() const { return 0; }
};

struct WatchableStatus {
    std::string name;
    bool alive;
    long memoryKB;
};