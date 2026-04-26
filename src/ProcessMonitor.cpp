#include "ProcessMonitor.h"

#include <cerrno>
#include <csignal>
#include <fstream>
#include <string>

static std::string getProcessName(pid_t targetPid) {
    std::string path = "/proc/" + std::to_string(targetPid) + "/comm";
    std::ifstream file(path);
    std::string processName;

    if (file.is_open() && std::getline(file, processName) && !processName.empty()) {
        return processName;
    }

    return "UnknownProcess";
}

ProcessMonitor::ProcessMonitor(pid_t pid) : m_name(getProcessName(pid)), m_pid(pid) {}

std::string ProcessMonitor::name() const {
    return m_name;
}

bool ProcessMonitor::isAlive() const {
    if (kill(m_pid, 0) == -1) {
        if (errno == ESRCH) {
            return false;
        }
    }
    return true;
}
