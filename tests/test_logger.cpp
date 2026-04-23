#include "../include/logger.h"
#include <cassert>
#include <sstream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>

// --- stream constructor ---

void testStreamConstructorLogsSingleMessage() {
    std::ostringstream oss;
    Logger logger(oss);
    logger.log("hello");
    assert(oss.str() == "hello\n");
}

void testStreamConstructorLogsMultipleMessages() {
    std::ostringstream oss;
    Logger logger(oss);
    logger.log("first");
    logger.log("second");
    assert(oss.str() == "first\nsecond\n");
}

// --- file constructor ---

void testFileConstructorCreatesAndWrites() {
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
    assert(line1 == "line one");
    assert(line2 == "line two");

    std::filesystem::remove(path);
}

void testFileConstructorAppendsOnReopen() {
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
    assert(line1 == "first session");
    assert(line2 == "second session");

    std::filesystem::remove(path);
}

void testFileConstructorThrowsOnBadPath() {
    bool threw = false;
    try {
        Logger logger("/nonexistent_dir/file.log");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

// --- thread safety ---

void testLogIsThreadSafe() {
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
    assert(newlines == numThreads * msgsPerThread);
}

int main() {
    testStreamConstructorLogsSingleMessage();
    testStreamConstructorLogsMultipleMessages();
    testFileConstructorCreatesAndWrites();
    testFileConstructorAppendsOnReopen();
    testFileConstructorThrowsOnBadPath();
    testLogIsThreadSafe();
    return 0;
}
