#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>

#include "../include/watchdog.h"
#include "../include/watchable.h"

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

// Helper: redirect std::cout into a stringstream for the duration of a scope
class CoutRedirect {
public:
    explicit CoutRedirect(std::ostream& target)
        : m_old(std::cout.rdbuf(target.rdbuf())) {}

    ~CoutRedirect() { std::cout.rdbuf(m_old); }

private:
    std::streambuf* m_old;
};

}  // namespace

TEST_CASE("Watchdog starts empty", "[watchdog]") {
    Watchdog wd;

    REQUIRE(wd.count() == 0);
    REQUIRE_FALSE(wd.isRunning());
}

TEST_CASE("Registering watchables increases count",
          "[watchdog][register]") {
    Watchdog wd;

    wd.registerWatchable(std::make_shared<FakeWatchable>("first", true));
    wd.registerWatchable(std::make_shared<FakeWatchable>("second", false));

    REQUIRE(wd.count() == 2);
}

TEST_CASE("start() and stop() change running state",
          "[watchdog][lifecycle]") {
    Watchdog wd;

    wd.start(10);
    REQUIRE(wd.isRunning());

    wd.stop();
    REQUIRE_FALSE(wd.isRunning());
}

TEST_CASE("Monitor loop logs status of registered watchables",
          "[watchdog][monitoring]") {
    std::ostringstream captured;
    CoutRedirect redirect(captured);

    Watchdog wd;
    wd.registerWatchable(std::make_shared<FakeWatchable>("alpha", true));
    wd.registerWatchable(std::make_shared<FakeWatchable>("beta", false));

    wd.start(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    wd.stop();

    const std::string output = captured.str();

    // Both watchables should appear in the output
    REQUIRE(output.find("alpha") != std::string::npos);
    REQUIRE(output.find("beta") != std::string::npos);

    // Their statuses should be reported correctly
    REQUIRE(output.find("[alpha] ALIVE") != std::string::npos);
    REQUIRE(output.find("[beta] DEAD") != std::string::npos);
}

TEST_CASE("Calling start() twice is a no-op", "[watchdog][edge]") {
    Watchdog wd;

    wd.start(10);
    wd.start(10);  // Must not crash or spawn a second thread

    REQUIRE(wd.isRunning());
    wd.stop();
}

TEST_CASE("Calling stop() without start() is safe",
          "[watchdog][edge]") {
    Watchdog wd;

    wd.stop();  // Must not crash

    REQUIRE_FALSE(wd.isRunning());
}

TEST_CASE("Destructor stops a running watchdog cleanly",
          "[watchdog][raii]") {
    {
        Watchdog wd;
        wd.registerWatchable(
            std::make_shared<FakeWatchable>("alive", true));
        wd.start(10);
        // Watchdog goes out of scope while running
    }
    // No crash, no leaked thread = test passes
    SUCCEED("Destructor handled running watchdog without crashing");
}