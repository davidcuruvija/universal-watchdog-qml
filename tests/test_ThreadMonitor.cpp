#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include "ThreadMonitor.h"

using namespace std::chrono_literals;

TEST_CASE("ThreadMonitor stores timeout and name from constructor", "[threadMonitor][construction]") {
    const auto timeout = 100ms;
    const std::string threadName = "worker-1";

    ThreadMonitor tm(timeout, threadName);

    REQUIRE(tm.timeout() == timeout);
    REQUIRE(tm.name() == threadName);
}


TEST_CASE("ThreadMonitor is alive immediately after construction", "[threadMonitor][isAlive]") {
    ThreadMonitor tm(100ms, "fresh");

    bool result = tm.isAlive();

    REQUIRE(result);
}

TEST_CASE("ThreadMonitor reports dead after timeout elapses without heartbeat", "[threadMonitor][isAlive][timeout]") {
    ThreadMonitor tm(20ms, "stale");

    std::this_thread::sleep_for(80ms);
    bool result = tm.isAlive();

    REQUIRE_FALSE(result);
}

TEST_CASE("ThreadMonitor::updateHeartbeat extends the alive window", "[threadMonitor][heartbeat]") {
    ThreadMonitor tm(50ms, "beating");

    std::this_thread::sleep_for(30ms);
    REQUIRE(tm.isAlive());

    tm.updateHeartbeat();

    std::this_thread::sleep_for(30ms);
    REQUIRE(tm.isAlive());

    std::this_thread::sleep_for(80ms);
    REQUIRE_FALSE(tm.isAlive());
}

TEST_CASE("ThreadMonitor works polymorphically through a Watchable pointer", "[threadMonitor][polymorphism]") {
    std::unique_ptr<Watchable> w = std::make_unique<ThreadMonitor>(100ms, "via-base");

    bool aliveResult = w->isAlive();
    std::string nameResult = w->name();

    REQUIRE(aliveResult);
    REQUIRE(nameResult == "via-base");
}

TEST_CASE("ThreadMonitor handles concurrent heartbeat and isAlive calls", "[threadMonitor][concurrency]") {
    ThreadMonitor tm(1000ms, "concurrent");
    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&] {
            while (!stop.load()) {
                tm.updateHeartbeat();
            }
        });
    }

    threads.emplace_back([&] {
        while (!stop.load()) {
            (void)tm.isAlive();
        }
    });

    std::this_thread::sleep_for(200ms);
    stop.store(true);
    for (auto& t : threads) t.join();

    REQUIRE(tm.isAlive());
}