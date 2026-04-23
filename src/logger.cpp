#include "logger.h"
#include <stdexcept>

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
    m_outStream << msg << '\n';
    m_outStream.flush();
}