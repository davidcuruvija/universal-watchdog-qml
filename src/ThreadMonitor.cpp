#include "ThreadMonitor.h"

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
}

bool ThreadMonitor::isAlive() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto elapsed = std::chrono::steady_clock::now() - m_lastHeartbeat;
    return elapsed < m_timeout;
}

std::string ThreadMonitor::name() const {
    return m_threadName;
}