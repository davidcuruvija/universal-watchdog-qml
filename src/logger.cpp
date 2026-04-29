#include "logger.h"
#include <stdexcept>
#include <chrono>
#include <ctime>

static std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string("[") + buf + "] ";
}

Logger::Logger(std::string fileName)
    : m_ownedFile(std::make_unique<std::ofstream>(fileName, std::ios::app))
    , m_outStream(*m_ownedFile)
    , m_mutex()
{
    if (!m_ownedFile->is_open()) {
        throw std::runtime_error("Logger: failed to open file '" + fileName + "'");
    }
}

Logger::Logger(std::ostream& externalStream)
    : m_ownedFile(nullptr)
    , m_outStream(externalStream)
    , m_mutex()
{
}

void Logger::log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_outStream << currentTimestamp() << msg << '\n';
    m_outStream.flush();
}