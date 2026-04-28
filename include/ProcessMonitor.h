#pragma once

#include "watchable.h"

#include <sys/types.h>

class ProcessMonitor : public Watchable {
private:
  std::string m_name;
  pid_t m_pid;

protected:
  virtual std::string getStatPath() const {
    return "/proc/" + std::to_string(m_pid) + "/stat";
  }

public:
  explicit ProcessMonitor(pid_t pid);

  std::string name() const override;
  bool isAlive() const override;
};
