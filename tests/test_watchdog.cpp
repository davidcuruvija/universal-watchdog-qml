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

// Helper: build a Config tailored for fast tests
Config testConfig(int pollMs = 10, int summaryMs = 60000) {
    Config c;
    c.pollInterval = std::chrono::milliseconds(pollMs);
    c.summaryInterval = std::chrono::milliseconds(summaryMs);
    return c;
}

}  // namespace

TEST_CASE("Watchdog starts empty", "[watchdog]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger, testConfig());

    REQUIRE(wd.count() == 0);
    REQUIRE_FALSE(wd.isRunning());
}

TEST_CASE("Registering watchables increases count",
          "[watchdog][register]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger, testConfig());

    wd.registerWatchable(std::make_shared<FakeWatchable>("first", true));
    wd.registerWatchable(std::make_shared<FakeWatchable>("second", false));

    REQUIRE(wd.count() == 2);
}

TEST_CASE("start() and stop() change running state",
          "[watchdog][lifecycle]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger, testConfig());

    wd.start();
    REQUIRE(wd.isRunning());

    wd.stop();
    REQUIRE_FALSE(wd.isRunning());
}

TEST_CASE("Monitor loop logs status of registered watchables",
          "[watchdog][monitoring]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger, testConfig());

    wd.registerWatchable(std::make_shared<FakeWatchable>("alpha", true));
    wd.registerWatchable(std::make_shared<FakeWatchable>("beta", false));

    wd.start();
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
    Watchdog wd(logger, testConfig());

    wd.start();
    wd.start();  // Must not crash or spawn a second thread

    REQUIRE(wd.isRunning());
    wd.stop();
}

TEST_CASE("Calling stop() without start() is safe",
          "[watchdog][edge]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger, testConfig());

    wd.stop();  // Must not crash

    REQUIRE_FALSE(wd.isRunning());
}

TEST_CASE("Destructor stops a running watchdog cleanly",
          "[watchdog][raii]") {

    std::ostringstream oss;
    Logger logger(oss);
    {
        Watchdog wd(logger, testConfig());
        wd.registerWatchable(
            std::make_shared<FakeWatchable>("alive", true));
        wd.start();
        // Watchdog goes out of scope while running
    }
    // No crash, no leaked thread = test passes
    SUCCEED("Destructor handled running watchdog without crashing");
}

TEST_CASE("stop() returns quickly even with long interval",
          "[watchdog][stop][performance]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger, testConfig(60000));

    wd.registerWatchable(std::make_shared<FakeWatchable>("worker", true));
    wd.start();   // 60-second interval 

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

TEST_CASE("unregisterByName removes a matching watchable",
          "[watchdog][unregister]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger, testConfig());

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
    Watchdog wd(logger, testConfig());

    wd.registerWatchable(std::make_shared<FakeWatchable>("alpha", true));

    const bool removed = wd.unregisterByName("nonexistent");

    REQUIRE_FALSE(removed);
    REQUIRE(wd.count() == 1);
}

TEST_CASE("registerWatchable rejects duplicates by name",
          "[watchdog][register][edge]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger, testConfig());

    REQUIRE(wd.registerWatchable(std::make_shared<FakeWatchable>("worker", true)));
    REQUIRE_FALSE(wd.registerWatchable(std::make_shared<FakeWatchable>("worker", false)));
    REQUIRE(wd.count() == 1);
    REQUIRE(wd.unregisterByName("worker"));
    REQUIRE(wd.count() == 0);
    REQUIRE_FALSE(wd.unregisterByName("worker"));
}

TEST_CASE("unregisterByName works while the monitor loop is running",
          "[watchdog][unregister][threading]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger, testConfig());

    wd.registerWatchable(std::make_shared<FakeWatchable>("alpha", true));
    wd.registerWatchable(std::make_shared<FakeWatchable>("beta", true));

    wd.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    // Remove while running — must not crash, must not deadlock
    const bool removed = wd.unregisterByName("alpha");

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    wd.stop();

    REQUIRE(removed);
    REQUIRE(wd.count() == 1);
}

TEST_CASE("Logs only on state change",
          "[watchdog][state-change]") {
    std::ostringstream oss;
    Logger logger(oss);
    // Long summary interval so it doesn't fire during the test
    Watchdog wd(logger, testConfig(10, 60000));

    auto target = std::make_shared<FakeWatchable>("worker", true);
    wd.registerWatchable(target);

    wd.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    wd.stop();

    const std::string output = oss.str();

    // Count occurrences of "[worker] ALIVE"
    size_t aliveCount = 0;
    size_t pos = 0;
    while ((pos = output.find("[worker] ALIVE", pos)) != std::string::npos) {
        ++aliveCount;
        ++pos;
    }

    // Should be exactly 1 (initial check), maybe 2 due to timing edge cases.
    // Definitely not 10+ which the old behavior would produce.
    REQUIRE(aliveCount >= 1);
    REQUIRE(aliveCount <= 3);
}

TEST_CASE("Detects state transitions",
          "[watchdog][state-change]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger, testConfig(10, 60000));

    auto target = std::make_shared<FakeWatchable>("worker", true);
    wd.registerWatchable(target);

    wd.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    target->setAlive(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    target->setAlive(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    wd.stop();

    const std::string output = oss.str();

    // Should see ALIVE, then DEAD, then ALIVE again
    const auto firstAlive = output.find("[worker] ALIVE");
    REQUIRE(firstAlive != std::string::npos);

    const auto firstDead = output.find("[worker] DEAD");
    REQUIRE(firstDead != std::string::npos);
    REQUIRE(firstAlive < firstDead);

    const auto secondAlive = output.find("[worker] ALIVE", firstDead);
    REQUIRE(secondAlive != std::string::npos);
}

TEST_CASE("Periodic summary logs even without state change",
          "[watchdog][summary]") {
    std::ostringstream oss;
    Logger logger(oss);
    // Very short summary interval so test completes quickly
    Watchdog wd(logger, testConfig(/*pollMs=*/10, /*summaryMs=*/30));

    wd.registerWatchable(std::make_shared<FakeWatchable>("stable", true));

    wd.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    wd.stop();

    const std::string output = oss.str();

    // With pollInterval=10 and summaryInterval=30, over 120ms we expect:
    // initial check + at least 2-3 summaries
    size_t aliveCount = 0;
    size_t pos = 0;
    while ((pos = output.find("[stable] ALIVE", pos)) != std::string::npos) {
        ++aliveCount;
        ++pos;
    }

    // Should be more than just the initial entry
    REQUIRE(aliveCount >= 2);
}

TEST_CASE("Watchdog works with real ThreadMonitor",
          "[watchdog][integration]") {
    std::ostringstream oss;
    Logger logger(oss);
    Watchdog wd(logger, testConfig(100));

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

    wd.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    wd.stop();

    workerRunning = false;
    worker.join();

    const std::string output = oss.str();

    REQUIRE(output.find("[live-thread] ALIVE") != std::string::npos);
    REQUIRE(output.find("[dead-thread] DEAD") != std::string::npos);
}