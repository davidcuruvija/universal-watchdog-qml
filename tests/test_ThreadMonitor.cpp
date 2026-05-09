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

#include <queue>
#include <condition_variable>

TEST_CASE("ThreadMonitor state: ALIVE", "[threadMonitor][states]") {
    ThreadMonitor tm(100ms, "AliveThread");
    std::atomic<bool> stop{false};
    std::queue<int> tasks;
    std::mutex mtx;
    std::condition_variable cv;

    std::thread worker([&]() {
        while (!stop) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait_for(lock, 30ms, [&]{ return !tasks.empty() || stop; });
                if (!tasks.empty()) tasks.pop();
            }
            tm.updateHeartbeat();
        }
    });

    for (int i = 0; i < 3; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.push(i);
        }
        cv.notify_one();
        std::this_thread::sleep_for(40ms);
        REQUIRE(tm.isAlive() == true);
    }

    stop = true;
    cv.notify_one();
    worker.join();
}

TEST_CASE("ThreadMonitor state: STUCK", "[threadMonitor][states]") {
    ThreadMonitor tm(100ms, "StuckThread");
    std::mutex mtx;
    std::atomic<bool> thread_started{false};

    std::unique_lock<std::mutex> main_lock(mtx);

    std::thread t([&]() {
        tm.updateHeartbeat();
        thread_started = true;
        std::lock_guard<std::mutex> lock(mtx);
        tm.updateHeartbeat();
    });

    while(!thread_started) { std::this_thread::yield(); }

    std::this_thread::sleep_for(150ms);
    
    REQUIRE(tm.isAlive() == false);

    main_lock.unlock();
    t.join();
}

TEST_CASE("ThreadMonitor state: DEAD", "[threadMonitor][states]") {
    ThreadMonitor tm(100ms, "DeadThread");
    
    std::thread t([&]() {
        tm.updateHeartbeat();
        std::this_thread::sleep_for(20ms);
    });

    t.join(); 
    
    REQUIRE(tm.isAlive() == false); 
}

TEST_CASE("ThreadMonitor state: ZOMBIE", "[threadMonitor][states]") {
    ThreadMonitor tm(100ms, "ZombieThread");
    
    std::thread t([&]() {
        tm.updateHeartbeat();
        std::this_thread::sleep_for(20ms);
    });

    std::this_thread::sleep_for(50ms);
    
    t.detach();
    REQUIRE(tm.isAlive() == false); 
}