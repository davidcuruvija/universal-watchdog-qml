#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>

#include "../include/watchdog.h"
#include "../include/watchable.h"
#include "../include/logger.h"
#include "../include/ThreadMonitor.h"
#include "../include/Config.h"

namespace {

// Test fixture: a watchable whose isAlive() result we can control
class FakeWatchable : public Watchable {
public:
    FakeWatchable(std::string name, bool alive)
        : m_name(std::move(name)), m_alive(alive) {}

    bool isAlive() const override { return m_alive; }
    std::string name() const override { return m_name; }

    void setAlive(bool alive) { m_alive = alive; }

private:
    std::string m_name;
    bool m_alive;
};

}  // namespace

TEST_CASE("Watchdog starts empty", "[watchdog]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    REQUIRE(wd.count() == 0);
    REQUIRE_FALSE(wd.isRunning());
}

TEST_CASE("Registering watchables increases count",
          "[watchdog][register]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    wd.registerWatchable(std::make_shared<FakeWatchable>("first", true));
    wd.registerWatchable(std::make_shared<FakeWatchable>("second", false));

    REQUIRE(wd.count() == 2);
}

TEST_CASE("start() and stop() change running state",
          "[watchdog][lifecycle]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    wd.start(10);
    REQUIRE(wd.isRunning());

    wd.stop();
    REQUIRE_FALSE(wd.isRunning());
}

TEST_CASE("Monitor loop logs status of registered watchables",
          "[watchdog][monitoring]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    wd.registerWatchable(std::make_shared<FakeWatchable>("alpha", true));
    wd.registerWatchable(std::make_shared<FakeWatchable>("beta", false));

    wd.start(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    wd.stop();

    const std::string output = oss.str();

    // Their statuses should be reported correctly
    REQUIRE(output.find("[alpha] ALIVE") != std::string::npos);
    REQUIRE(output.find("[beta] DEAD") != std::string::npos);
}

TEST_CASE("Calling start() twice is a no-op", "[watchdog][edge]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    wd.start(10);
    wd.start(10);  // Must not crash or spawn a second thread

    REQUIRE(wd.isRunning());
    wd.stop();
}

TEST_CASE("Calling stop() without start() is safe",
          "[watchdog][edge]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    wd.stop();  // Must not crash

    REQUIRE_FALSE(wd.isRunning());
}

TEST_CASE("Destructor stops a running watchdog cleanly",
          "[watchdog][raii]") {

    std::ostringstream oss;
    Logger logger(oss);
    {
        Watchdog wd(logger);
        wd.registerWatchable(
            std::make_shared<FakeWatchable>("alive", true));
        wd.start(10);
        // Watchdog goes out of scope while running
    }
    // No crash, no leaked thread = test passes
    SUCCEED("Destructor handled running watchdog without crashing");
}

TEST_CASE("stop() returns quickly even with long interval",
          "[watchdog][stop][performance]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    wd.registerWatchable(std::make_shared<FakeWatchable>("worker", true));
    wd.start(60000);   // 60-second interval 

    // Allow one iteration to log
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const auto stopStart = std::chrono::steady_clock::now();
    wd.stop();
    const auto stopEnd = std::chrono::steady_clock::now();

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        stopEnd - stopStart).count();

    // stop() should return in well under a second, despite 60-second interval
    REQUIRE(elapsed < 1000);
}

TEST_CASE("Watchdog::start(Config) uses pollInterval from config",
          "[watchdog][config]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    wd.registerWatchable(std::make_shared<FakeWatchable>("worker", true));

    Config config;
    config.pollInterval = std::chrono::milliseconds(10);

    wd.start(config);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    wd.stop();

    REQUIRE(oss.str().find("[worker] ALIVE") != std::string::npos);
}

TEST_CASE("unregisterByName removes a matching watchable",
          "[watchdog][unregister]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    wd.registerWatchable(std::make_shared<FakeWatchable>("alpha", true));
    wd.registerWatchable(std::make_shared<FakeWatchable>("beta", true));
    REQUIRE(wd.count() == 2);

    const bool removed = wd.unregisterByName("alpha");

    REQUIRE(removed);
    REQUIRE(wd.count() == 1);
}

TEST_CASE("unregisterByName returns false for unknown name",
          "[watchdog][unregister]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    wd.registerWatchable(std::make_shared<FakeWatchable>("alpha", true));

    const bool removed = wd.unregisterByName("nonexistent");

    REQUIRE_FALSE(removed);
    REQUIRE(wd.count() == 1);
}

TEST_CASE("unregisterByName removes only the first match when duplicates exist",
          "[watchdog][unregister][edge]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    wd.registerWatchable(std::make_shared<FakeWatchable>("worker", true));
    wd.registerWatchable(std::make_shared<FakeWatchable>("worker", false));
    REQUIRE(wd.count() == 2);

    const bool removed = wd.unregisterByName("worker");

    REQUIRE(removed);
    REQUIRE(wd.count() == 1);

    // Second call should also succeed (the duplicate is still there)
    REQUIRE(wd.unregisterByName("worker"));
    REQUIRE(wd.count() == 0);
}

TEST_CASE("unregisterByName works while the monitor loop is running",
          "[watchdog][unregister][threading]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    wd.registerWatchable(std::make_shared<FakeWatchable>("alpha", true));
    wd.registerWatchable(std::make_shared<FakeWatchable>("beta", true));

    wd.start(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    // Remove while running — must not crash, must not deadlock
    const bool removed = wd.unregisterByName("alpha");

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    wd.stop();

    REQUIRE(removed);
    REQUIRE(wd.count() == 1);
}

TEST_CASE("Watchdog works with real ThreadMonitor",
          "[watchdog][integration]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger);

    // ThreadMonitor whose heartbeat is refreshed by a worker thread
    auto liveMonitor = std::make_shared<ThreadMonitor>(
        std::chrono::milliseconds(500), "live-thread");

    // ThreadMonitor whose heartbeat is never refreshed — will appear dead
    // after its short timeout expires
    auto deadMonitor = std::make_shared<ThreadMonitor>(
        std::chrono::milliseconds(10), "dead-thread");

    wd.registerWatchable(liveMonitor);
    wd.registerWatchable(deadMonitor);

    // Simulate a healthy worker that keeps refreshing its heartbeat
    std::atomic<bool> workerRunning{true};
    std::thread worker([&liveMonitor, &workerRunning]() {
        while (workerRunning.load()) {
            liveMonitor->updateHeartbeat();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    wd.start(100);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    wd.stop();

    workerRunning = false;
    worker.join();

    const std::string output = oss.str();

    REQUIRE(output.find("[live-thread] ALIVE") != std::string::npos);
    REQUIRE(output.find("[dead-thread] DEAD") != std::string::npos);
}