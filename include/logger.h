#pragma once
#include <string>
#include <fstream>
#include <ostream>
#include <mutex>
#include <memory>

class Logger {
    public:
        explicit Logger(const std::string& fileName);
        explicit Logger(std::ostream& externalStream);

        ~Logger() = default;

        Logger(const Logger&) = delete;
        Logger& operator =(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator =(Logger&&) = delete;

        void log(const std::string& msg);

    private:
        std::unique_ptr<std::ofstream> m_ownedFile;
        std::ostream& m_outStream;
        std::mutex m_mutex;
};