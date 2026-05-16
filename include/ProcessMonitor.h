#pragma once

#include "Manageable.h"

#include <string>
#include <vector>
#include <sys/types.h>

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
  pid_t m_pid;
  std::string m_processName;
  std::string m_name;
  std::vector<std::string> m_cmdline; ///< Original argv, captured at construction for restart.

  std::string getStatPath() const {
    return "/proc/" + std::to_string(m_pid) + "/stat";
  }

  static std::vector<std::string> readCmdline(pid_t pid);

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
};
