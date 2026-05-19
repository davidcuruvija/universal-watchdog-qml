#pragma once
#include <string>
#include <vector>
#include <set>
#include <utility>
#include "watchable.h"

/**
 * @brief Handles colorized, formatted terminal output for the watchdog REPL.
 */
class TerminalRenderer {
public:
    void printBanner() const;
    void printPrompt() const;
    void printHelp() const;
    void printWatchHeader(const std::string& timeStr, bool isLive = true) const;
    void printList(const std::vector<WatchableStatus>& entries, const std::set<std::string>& changedEntities = {}) const;
    void printSuccess(const std::string& msg) const;
    void printWarning(const std::string& msg) const;
    void printError(const std::string& msg) const;
    void printInfo(const std::string& msg) const;
    void printConfig(const std::string& logFile,
                     int heartbeatMs, int pollMs, int summaryMs) const;
    void clearAll() const;
    void clearVisible() const;
    void moveCursorUp(int lines) const;
    void clearFromCursor() const;
};
