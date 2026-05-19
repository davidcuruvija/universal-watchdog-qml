#pragma once
#include "watchable.h"

/**
 * @brief Extension of Watchable for objects that support active management.
 *
 * Implementors can be killed (sent SIGTERM) and restarted (killed then
 * relaunched with the original command line).
 */
class Manageable : public Watchable {
public:
    /**
     * @brief Send SIGTERM to the monitored process.
     * @return true if the signal was delivered successfully.
     */
    virtual bool kill() = 0;

    /**
     * @brief Kill and relaunch the process with its original command line.
     *
     * Sends SIGTERM, waits up to one second, then SIGKILL if needed, then
     * forks and execs the original argv. Updates the internal PID on success.
     *
     * @return true if the new process was spawned successfully.
     */
    virtual bool restart() = 0;
};
