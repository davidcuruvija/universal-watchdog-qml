#pragma once

#include "Manageable.h"

#include <chrono>
#include <string>
#include <vector>
#include <sys/types.h>

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
 * Implements Manageable so the process can be killed or restarted through
 * the watchdog interface. The original command line is captured at
 * construction from /proc/<pid>/cmdline and used for restart.
 */
class ProcessMonitor : public Manageable {
private:
  pid_t m_pid;        ///< The Process ID (PID) of the monitored process.
  std::string m_processName;  ///< Raw process name
  std::string m_name; ///< Name including PID (e.g. "firefox (pid=1234)")
  std::vector<std::string> m_cmdline; ///< Original argv, captured at construction for restart.

  // D-state tracking
  mutable bool m_inDState = false;
  mutable std::chrono::steady_clock::time_point m_dStateSince;

  /**
   * @brief Reads the raw state character from /proc/<pid>/stat.
   * @return The state char ('R','S','D','T','Z') or '\0' if unreadable.
   */
  char readRawState() const;

  std::string getStatPath() const {
    return "/proc/" + std::to_string(m_pid) + "/stat";
  }

  static std::vector<std::string> readCmdline(pid_t pid);
  static long readCpuTicks(pid_t pid);

  mutable long m_prevCpuTicks{-1};
  mutable std::chrono::steady_clock::time_point m_prevCpuTime;

 public:
  /**
   * @brief Constructs a ProcessMonitor for the specified PID.
   *
   * Reads the process name and command line from /proc. Throws
   * std::invalid_argument if the process does not exist.
   */
  explicit ProcessMonitor(pid_t pid);

  std::string name() const override;

  /**
   * @brief Returns true if the process is in R, S, or D state.
   */
  bool isAlive() const override;

  /**
   * @brief Sends SIGTERM to the monitored process.
   * @return true if ::kill() succeeded.
   */
  bool kill() override;

  /**
   * @brief Kills the process and relaunches it with the original command line.
   *
   * Waits up to one second for the process to exit after SIGTERM, then sends
   * SIGKILL. Forks and execs the stored argv. Updates the tracked PID.
   *
   * @return true if a new child process was spawned.
   * @return false if the original command line is unavailable or fork failed.
   */
  bool restart() override;

  pid_t pid() const { return m_pid; }

  /**
   * @brief Returns the resident set size (VmRSS) in kilobytes from /proc.
   * Returns 0 if the process is not alive or the value cannot be read.
   */
  long memoryKB() const override;

  /**
   * @brief Returns CPU usage percentage since the last call.
   *
   * Reads utime+stime from /proc/<pid>/stat and computes a percentage over
   * the elapsed wall-clock time. The first call seeds the baseline and
   * returns 0. Returns 0 if the process is unreachable.
   */
  double cpuPercent() const override;

  /**
   * @brief Returns the granular process state.
   *
   * Uses the raw state character from /proc/<pid>/stat and applies
   * D-state timeout logic to distinguish healthy disk-sleep from stuck.
   */
  ProcessState getProcessState() const;
};
