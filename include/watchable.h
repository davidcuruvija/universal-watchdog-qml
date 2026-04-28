#pragma once

#include <string>

/**
 * @class Watchable
 * @brief Abstract interface for objects that can be monitored by the Watchdog
 * Engine.
 *
 * Any class that needs to be monitored (e.g., OS processes or individual
 * threads) must implement this interface. The Watchdog Engine will maintain a
 * list of Watchable objects and periodically check their status.
 */
class Watchable {
public:
  /**
   * @brief Virtual destructor for proper cleanup of derived classes.
   */
  virtual ~Watchable() = default;

  /**
   * @brief Checks if the watched entity is still alive and operational.
   * @return true If the entity is alive and functioning correctly.
   * @return false If the entity has crashed, frozen, or terminated.
   */
  virtual bool isAlive() const = 0;

  /**
   * @brief Returns the identifier name of the monitored object.
   *
   * This name is used by the Watchdog Engine for logging and reporting
   * the status of different monitored entities.
   *
   * @return std::string The name of the process, thread, or component.
   */
  virtual std::string name() const = 0;
};