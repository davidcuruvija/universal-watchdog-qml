#pragma once
#include "watchable.h"
#include <string>
#include <chrono>
#include <mutex>

class ThreadMonitor : public Watchable {
public:
    ThreadMonitor(std::chrono::milliseconds timeout, std::string threadName);

    ThreadMonitor(const ThreadMonitor&) = delete;
    ThreadMonitor& operator=(const ThreadMonitor&) = delete;
    ThreadMonitor(ThreadMonitor&&) = delete;
    ThreadMonitor& operator=(ThreadMonitor&&) = delete;
    ~ThreadMonitor() override = default;

    void updateHeartbeat();

    bool isAlive() override;
    std::string name() const override;

    std::chrono::milliseconds timeout() const { 
        return m_timeout; 
    }
    
private:
    const std::chrono::milliseconds m_timeout;
    const std::string m_threadName;
    std::chrono::steady_clock::time_point m_lastHeartbeat;
    mutable std::mutex m_mutex;
};