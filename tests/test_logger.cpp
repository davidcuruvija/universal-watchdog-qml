#include <catch2/catch_test_macros.hpp>
#include "../include/logger.h"
#include <sstream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>

// --- stream constructor ---

TEST_CASE("Logger with stream constructor logs a single message", "[logger][stream]") {
    // Arrange
    std::ostringstream oss;
    Logger logger(oss);

    // Act
    logger.log("hello");

    // Assert
    REQUIRE(oss.str() == "hello\n");
}

TEST_CASE("Logger with stream constructor logs multiple messages", "[logger][stream]") {
    // Arrange
    std::ostringstream oss;
    Logger logger(oss);

    // Act
    logger.log("first");
    logger.log("second");

    // Assert
    REQUIRE(oss.str() == "first\nsecond\n");
}

// --- file constructor ---

TEST_CASE("Logger with file constructor creates file and writes messages", "[logger][file]") {
    // Arrange
    const std::string path = "/tmp/test_logger_create.log";
    std::filesystem::remove(path);

    // Act
    {
        Logger logger(path);
        logger.log("line one");
        logger.log("line two");
    }

    // Assert
    std::ifstream f(path);
    std::string line1, line2;
    std::getline(f, line1);
    std::getline(f, line2);
    REQUIRE(line1 == "line one");
    REQUIRE(line2 == "line two");

    std::filesystem::remove(path);
}

TEST_CASE("Logger with file constructor appends on reopen", "[logger][file]") {
    // Arrange
    const std::string path = "/tmp/test_logger_append.log";
    std::filesystem::remove(path);

    // Act
    {
        Logger logger(path);
        logger.log("first session");
    }
    {
        Logger logger(path);
        logger.log("second session");
    }

    // Assert
    std::ifstream f(path);
    std::string line1, line2;
    std::getline(f, line1);
    std::getline(f, line2);
    REQUIRE(line1 == "first session");
    REQUIRE(line2 == "second session");

    std::filesystem::remove(path);
}

TEST_CASE("Logger with file constructor throws on bad path", "[logger][file]") {
    // Arrange
    const std::string badPath = "/nonexistent_dir/file.log";

    // Act & Assert
    REQUIRE_THROWS_AS(Logger(badPath), std::runtime_error);
}

// --- thread safety ---

TEST_CASE("Logger::log is thread-safe", "[logger][thread]") {
    // Arrange
    std::ostringstream oss;
    Logger logger(oss);

    constexpr int numThreads = 8;
    constexpr int msgsPerThread = 50;

    // Act
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&logger, i]() {
            for (int j = 0; j < msgsPerThread; ++j) {
                logger.log("thread " + std::to_string(i) + " msg " + std::to_string(j));
            }
        });
    }
    for (auto& t : threads) t.join();

    // Assert
    const std::string out = oss.str();
    int newlines = 0;
    for (char c : out) if (c == '\n') ++newlines;
    REQUIRE(newlines == numThreads * msgsPerThread);
}
