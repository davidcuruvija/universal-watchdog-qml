#pragma once

#include "Watchable.h"

#include <sys/types.h>

class ProcessMonitor : public Watchable {
private:
    std::string m_name;
    pid_t m_pid;

public:
    explicit ProcessMonitor(pid_t pid);

    std::string getName() const override;
    bool isAlive() const override;
};
