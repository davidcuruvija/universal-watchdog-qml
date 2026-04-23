#include <catch2/catch_test_macros.hpp>
#include <memory>
#include "AlwaysAliveWatchable.h"

namespace {
    class TrackingWatchable : public AlwaysAliveWatchable {
    public:
        explicit TrackingWatchable(bool& destroyed_flag)
            : destroyed_(destroyed_flag) {}
        ~TrackingWatchable() override {
            destroyed_ = true;
        }
    private:
        bool& destroyed_;
    };
}

TEST_CASE("AlwaysAliveWatchable::isAlive() returns true", "[watchable][isAlive]") {
    AlwaysAliveWatchable w;

    bool result = w.isAlive();

    REQUIRE(result);
}

TEST_CASE("AlwaysAliveWatchable::name() returns expected string", "[watchable][name]") {
    AlwaysAliveWatchable w;

    std::string actualName = w.name();

    REQUIRE(actualName == "always-alive");
}

TEST_CASE("Virtual destructor is called when destroyed via base class", "[watchable][destructor]") {
    bool destroyed = false;
    
    {
        auto ptr = std::make_unique<TrackingWatchable>(destroyed);
    }

    INFO("If this fails, check that ~Watchable() is marked 'virtual'");
    REQUIRE(destroyed);
}

TEST_CASE("Polymorphic calls work through Watchable base pointer", "[watchable][polymorphism]") {
    std::unique_ptr<Watchable> w = std::make_unique<AlwaysAliveWatchable>();

    bool aliveResult = w->isAlive();
    std::string nameResult = w->name();

    CHECK(aliveResult);
    CHECK(nameResult == "always-alive");
}