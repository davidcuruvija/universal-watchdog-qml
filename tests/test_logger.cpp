#include <catch2/catch_test_macros.hpp>
#include "../include/logger.h"
#include "../include/Config.h"
#include <sstream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>

static bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.substr(str.size() - suffix.size()) == suffix;
}

TEST_CASE("Logger with stream constructor logs a single message", "[logger][stream]") {
    std::ostringstream oss;
    Logger logger(oss);

    logger.log("hello");

    REQUIRE(endsWith(oss.str(), "hello\n"));
}

TEST_CASE("Logger with stream constructor logs multiple messages", "[logger][stream]") {
    std::ostringstream oss;
    Logger logger(oss);

    logger.log("first");
    logger.log("second");

    std::istringstream ss(oss.str());
    std::string line1, line2;
    std::getline(ss, line1);
    std::getline(ss, line2);
    REQUIRE(endsWith(line1, "first"));
    REQUIRE(endsWith(line2, "second"));
}

TEST_CASE("Logger with file constructor creates file and writes messages", "[logger][file]") {
    const std::string path = "/tmp/test_logger_create.log";
    std::filesystem::remove(path);

    {
        Logger logger(path);
        logger.log("line one");
        logger.log("line two");
    }

    std::ifstream f(path);
    std::string line1, line2;
    std::getline(f, line1);
    std::getline(f, line2);
    REQUIRE(endsWith(line1, "line one"));
    REQUIRE(endsWith(line2, "line two"));

    std::filesystem::remove(path);
}

TEST_CASE("Logger with file constructor appends on reopen", "[logger][file]") {
    const std::string path = "/tmp/test_logger_append.log";
    std::filesystem::remove(path);

    {
        Logger logger(path);
        logger.log("first session");
    }
    {
        Logger logger(path);
        logger.log("second session");
    }

    std::ifstream f(path);
    std::string line1, line2;
    std::getline(f, line1);
    std::getline(f, line2);
    REQUIRE(endsWith(line1, "first session"));
    REQUIRE(endsWith(line2, "second session"));

    std::filesystem::remove(path);
}

TEST_CASE("Logger with file constructor throws on bad path", "[logger][file]") {
    const std::string badPath = "/nonexistent_dir/file.log";

    REQUIRE_THROWS_AS(Logger(badPath), std::runtime_error);
}


TEST_CASE("Logger with Config constructor writes to configured file", "[logger][config]") {
    const std::string path = "/tmp/test_logger_config.log";
    std::filesystem::remove(path);

    Config config;
    config.logFile = path;

    {
        Logger logger(config);
        logger.log("from config");
    }

    std::ifstream f(path);
    std::string line;
    std::getline(f, line);
    REQUIRE(endsWith(line, "from config"));

    std::filesystem::remove(path);
}


TEST_CASE("Logger::log is thread-safe", "[logger][thread]") {
    std::ostringstream oss;
    Logger logger(oss);

    constexpr int numThreads = 8;
    constexpr int msgsPerThread = 50;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&logger, i]() {
            for (int j = 0; j < msgsPerThread; ++j) {
                logger.log("thread " + std::to_string(i) + " msg " + std::to_string(j));
            }
        });
    }
    for (auto& t : threads) t.join();

    const std::string out = oss.str();
    int newlines = 0;
    for (char c : out) if (c == '\n') ++newlines;
    REQUIRE(newlines == numThreads * msgsPerThread);
}
