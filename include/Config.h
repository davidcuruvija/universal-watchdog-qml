#pragma once
#include <chrono>
#include <string>

/**
 * @brief Project-wide configuration parameters for the watchdog system.
 *
 * Holds sensible defaults for all configurable values. Pass a Config instance
 * to Logger, ThreadMonitor, and Watchdog instead of scattering
 * magic numbers across the codebase.
 */
struct Config {
  std::string logFile = "watchdog.log";
  std::chrono::milliseconds heartbeatTimeout{1000};
  std::chrono::milliseconds pollInterval{1000};
  std::chrono::milliseconds summaryInterval{60000};
};
