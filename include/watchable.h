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

  /**
   * @brief Returns CPU usage as a percentage (0–100+), or 0 if not applicable.
   *
   * Computed as a delta between successive calls; the first call always
   * returns 0 and seeds the baseline for subsequent measurements.
   */
  virtual double cpuPercent() const { return 0.0; }
};

struct WatchableStatus {
  std::string name;
  bool alive;
  long memoryKB;
  double cpuPercent;
  std::string statusLabel;
};