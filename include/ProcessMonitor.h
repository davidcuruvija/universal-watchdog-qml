#pragma once

#include "watchable.h"

#include <sys/types.h>
#include <chrono>

/**
 * @brief Granular process states derived from /proc/<pid>/stat.
 */
enum class ProcessState {
    ALIVE,    ///< R (Running) or S (Sleeping) — healthy
    STOPPED,  ///< T — process frozen by signal or debugger
    ZOMBIE,   ///< Z — terminated but not yet reaped
    DEAD,     ///< /proc/<pid>/stat does not exist
    STUCK     ///< D (Disk sleep) for longer than threshold
};

/**
 * @class ProcessMonitor
 * @brief Monitors a specific system process using its Process ID (PID).
 *
 * This class implements the Watchable interface to monitor the state
 * of system processes by reading from the /proc filesystem in Linux.
 */
class ProcessMonitor : public Watchable {
private:
  pid_t m_pid;        ///< The Process ID (PID) of the monitored process.
  std::string m_processName;  ///< Raw process name 
  std::string m_name; ///< Name including PID (e.g. "firefox (pid=1234)")
  
  // D-state tracking
  mutable bool m_inDState = false;
  mutable std::chrono::steady_clock::time_point m_dStateSince;

  /**
   * @brief Reads the raw state character from /proc/<pid>/stat.
   * @return The state char ('R','S','D','T','Z') or '\0' if unreadable.
   */
  char readRawState() const;

  /**
   * @brief Returns the absolute filesystem path to the process's status file.
   *
   * This path is used to retrieve system-level process information, typically
   * from the /proc filesystem on Linux systems.
   *
   * @return std::string The absolute path to the status file.
   */
  std::string getStatPath() const {
    return "/proc/" + std::to_string(m_pid) + "/stat";
  }

public:
  /**
   * @brief Constructs a ProcessMonitor for the specified PID.
   * @param pid The Process ID of the target process to monitor.
   */
  explicit ProcessMonitor(pid_t pid);

  /**
   * @brief Retrieves the name of the monitored process.
   * @return The process name extracted from the system.
   */
  std::string name() const override;

  /**
   * @brief Evaluates if the monitored process is currently alive.
   *
   * Parses the stat file to check the exact state character.
   *
   * @return true If the process is in Running (R), Sleeping (S), or Disk Sleep
   * (D) state.
   * @return false If the process does not exist, or is in Zombie (Z) or Stopped
   * (T) state.
   */
  bool isAlive() const override;

  /**
   * @brief Returns the granular process state.
   *
   * Uses the raw state character from /proc/<pid>/stat and applies
   * D-state timeout logic to distinguish healthy disk-sleep from stuck.
   */
  ProcessState getProcessState() const;
};
