#include "ThreadMonitor.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <filesystem>

ThreadMonitor::ThreadMonitor(const Config& config, std::string threadName)
    : ThreadMonitor(config.heartbeatTimeout, std::move(threadName))
{
}

ThreadMonitor::ThreadMonitor(std::chrono::milliseconds timeout, std::string threadName)
    : m_timeout(timeout)
    , m_threadName(std::move(threadName))
    , m_lastHeartbeat(std::chrono::steady_clock::now())
{
}

void ThreadMonitor::updateHeartbeat() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastHeartbeat = std::chrono::steady_clock::now();
    m_nativeTid = static_cast<pid_t>(syscall(SYS_gettid));
}

bool ThreadMonitor::isAlive() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_nativeTid.has_value()) {
        const std::string taskPath =
            "/proc/self/task/" + std::to_string(m_nativeTid.value());
        if (!std::filesystem::exists(taskPath)) {
            return false;
        }
    }

    const auto elapsed = std::chrono::steady_clock::now() - m_lastHeartbeat;
    return elapsed < m_timeout;
}

std::string ThreadMonitor::name() const {
    return m_threadName;
}