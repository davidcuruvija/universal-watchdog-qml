#pragma once
#include "Config.h"
#include <string>
#include <fstream>
#include <ostream>
#include <mutex>
#include <memory>

/**
 * @brief Thread-safe logger that writes timestamped messages to a stream.
 *
 * Can be constructed with either a file path (Logger owns the file) or an
 * external stream (e.g. std::cout). Not copyable or movable.
 */
class Logger {
public:
    /**
     * @brief Constructs a Logger from a Config, writing to the configured log file.
     * @param config Project configuration. Uses config.logFile as the output path.
     */
    explicit Logger(const Config& config);

    /**
     * @brief Constructs a Logger that writes to a file.
     * @param fileName Path to the output log file. Created if it does not exist.
     */
    explicit Logger(const std::string& fileName);

    /**
     * @brief Constructs a Logger that writes to an existing stream.
     * @param externalStream Stream to write to (e.g. std::cout, std::cerr).
     *        The caller must ensure the stream outlives this Logger.
     */
    explicit Logger(std::ostream& externalStream);

    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Writes a timestamped message to the output stream.
     * @param msg Message to log.
     */
    void log(const std::string& msg);

private:
    std::unique_ptr<std::ofstream> m_ownedFile;
    std::ostream& m_outStream;
    std::mutex m_mutex;
};
