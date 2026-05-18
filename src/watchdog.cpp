#include "../include/watchdog.h"
#include "../include/ProcessMonitor.h"

#include <chrono>
#include <iostream>
#include <algorithm>

Watchdog::Watchdog(Logger& logger, Config config)
    : m_logger(logger)
    , m_config(std::move(config))
    , m_lastSummary(std::chrono::steady_clock::now())
    , m_running(false) {
}

Watchdog::~Watchdog() {
    stop();  // Safety: ensure thread doesn't outlive the object
}

bool Watchdog::registerWatchable(std::shared_ptr<Watchable> w) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check for duplicates by name
    for (const auto& existing : m_watchables) {
        if (existing->name() == w->name()) {
            return false; // Already registered
        }
    }
    
    m_watchables.push_back(std::move(w));
    return true;
}

bool Watchdog::unregisterByName(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(
        m_watchables.begin(), m_watchables.end(),
        [&name](const std::shared_ptr<Watchable>& w) {
            return w->name() == name;
        });

    if (it == m_watchables.end()) {
        return false;
    }

    m_watchables.erase(it);
    m_lastStatus.erase(name);
    return true;
}


void Watchdog::start() {
    if (m_running.exchange(true)) {
        return;  // Already running
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastStatus.clear();
        m_lastSummary = std::chrono::steady_clock::now();
    }
    m_monitorThread = std::thread(&Watchdog::monitorLoop, this);
}

void Watchdog::stop() {
    if (!m_running.exchange(false)) {
        return;  // Was not running
    }

    // Wake up the monitor thread immediately if it's sleeping
    {
        std::lock_guard<std::mutex> lock(m_stopMutex);
    }
    m_stopCv.notify_all();

    if (m_monitorThread.joinable()) {
        m_monitorThread.join();
    }
}

bool Watchdog::isRunning() const {
    return m_running.load();
}

std::size_t Watchdog::count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_watchables.size();
}

std::vector<WatchableStatus> Watchdog::getStatusSnapshot() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<WatchableStatus> result;
    result.reserve(m_watchables.size());
    for (const auto& w : m_watchables) {
        result.push_back({w->name(), w->isAlive(), w->memoryKB(), w->cpuPercent()});
    }
    switch (m_sortMode) {
        case SortMode::CPU:
            std::sort(result.begin(), result.end(), [](const WatchableStatus& a, const WatchableStatus& b) {
                return a.cpuPercent > b.cpuPercent;
            });
            break;
        case SortMode::NAME:
            std::sort(result.begin(), result.end(), [](const WatchableStatus& a, const WatchableStatus& b) {
                return a.name < b.name;
            });
            break;
        case SortMode::MEMORY:
        default:
            std::sort(result.begin(), result.end(), [](const WatchableStatus& a, const WatchableStatus& b) {
                return a.memoryKB > b.memoryKB;
            });
            break;
    }
    return result;
}

void Watchdog::setSortMode(SortMode mode) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sortMode = mode;
}

std::vector<WatchableStatus> Watchdog::getDetailedSnapshot() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<WatchableStatus> result;
    result.reserve(m_watchables.size());

    for (const auto& w : m_watchables) {
        auto pm = std::dynamic_pointer_cast<ProcessMonitor>(w);

        WatchableStatus status{
            w->name(),
            w->isAlive(),
            w->memoryKB(),
            w->cpuPercent()
        };

        if (pm) {
            status.processState = pm->getProcessState();
        }

        result.push_back(status);
    }

    switch (m_sortMode) {
        case SortMode::CPU:
            std::sort(result.begin(), result.end(),
                [](const WatchableStatus& a, const WatchableStatus& b) {
                    return a.cpuPercent > b.cpuPercent;
                });
            break;

        case SortMode::NAME:
            std::sort(result.begin(), result.end(),
                [](const WatchableStatus& a, const WatchableStatus& b) {
                    return a.name < b.name;
                });
            break;

        case SortMode::MEMORY:
        default:
            std::sort(result.begin(), result.end(),
                [](const WatchableStatus& a, const WatchableStatus& b) {
                    return a.memoryKB > b.memoryKB;
                });
            break;
    }

    return result;
}

void Watchdog::monitorLoop() {
    while (m_running.load()) {
        checkAndLog();

        std::unique_lock<std::mutex> lock(m_stopMutex);
        m_stopCv.wait_for(lock, 
                        m_config.pollInterval,
                        [this] {return !m_running.load();});
    }
}

bool Watchdog::killByName(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& w : m_watchables) {
        if (w->name() == name) {
            auto mgr = std::dynamic_pointer_cast<Manageable>(w);
            return mgr && mgr->kill();
        }
    }
    return false;
}

bool Watchdog::restartByName(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& w : m_watchables) {
        if (w->name() == name) {
            auto mgr = std::dynamic_pointer_cast<Manageable>(w);
            if (!mgr) return false;
            std::string oldName = w->name();
            bool ok = mgr->restart();
            if (ok) m_lastStatus.erase(oldName);
            return ok;
        }
    }
    return false;
}

void Watchdog::checkAndLog() {
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto now = std::chrono::steady_clock::now();
    const bool summaryDue = (now - m_lastSummary) >= m_config.summaryInterval;

    for (const auto& w : m_watchables) {
        const std::string watchableName = w->name();
        
        std::string currentState;
        auto pm = std::dynamic_pointer_cast<ProcessMonitor>(w);
        if (pm) {
            ProcessState ps = pm->getProcessState();
            switch (ps) {
                case ProcessState::ALIVE:   currentState = "ALIVE";   break;
                case ProcessState::STOPPED: currentState = "STOPPED"; break;
                case ProcessState::ZOMBIE:  currentState = "ZOMBIE";  break;
                case ProcessState::DEAD:    currentState = "DEAD";    break;
                case ProcessState::STUCK:   currentState = "STUCK";   break;
            }
        } else {
            currentState = w->isAlive() ? "ALIVE" : "DEAD";
        }

        auto it = m_lastStatus.find(watchableName);
        const bool firstCheck = (it == m_lastStatus.end());
        const bool changed = !firstCheck && (it->second != currentState);

        const bool shouldLog = firstCheck || changed || summaryDue;

        if (shouldLog) {
            m_logger.log("[" + watchableName + "] " + currentState);
        }

        m_lastStatus[watchableName] = currentState;
    }

    if (summaryDue) {
        m_lastSummary = now;
    }
}