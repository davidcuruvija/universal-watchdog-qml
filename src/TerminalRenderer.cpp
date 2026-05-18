#include "../include/TerminalRenderer.h"
#include <iomanip>
#include <iostream>

namespace Color {
constexpr const char *RESET = "\033[0m";
constexpr const char *BOLD = "\033[1m";
constexpr const char *DIM = "\033[2m";
constexpr const char *RED = "\033[31m";
constexpr const char *GREEN = "\033[32m";
constexpr const char *YELLOW = "\033[33m";
constexpr const char *CYAN = "\033[36m";
constexpr const char *MAGENTA = "\033[35m";
constexpr const char *ORANGE = "\033[38;5;208m";
} // namespace Color

void TerminalRenderer::printBanner() const {
  std::cout << Color::CYAN << Color::BOLD
            << "╔═════════════════════════════════════════════════╗\n"
            << "║              🐕 UNIVERSAL WATCHDOG              ║\n"
            << "╚═════════════════════════════════════════════════╝\n"
            << Color::RESET << "  Type 'help' for available commands.\n\n";
}

void TerminalRenderer::printPrompt() const {
  std::cout << Color::BOLD << "watchdog> " << Color::RESET;
}

void TerminalRenderer::printHelp() const {
  std::cout
      << "\n  Available commands:\n"
      << "  ┌─────────────────────┬───────────────────────────────────┐\n"
      << "  │ " << Color::CYAN << "add process <pid>" << Color::RESET
      << "   │ Start monitoring a process by PID │\n"
      << "  │ " << Color::CYAN << "remove <name>" << Color::RESET
      << "       │ Stop monitoring by name           │\n"
      << "  │ " << Color::CYAN << "list" << Color::RESET
      << "                │ Show all monitored entities       │\n"
      << "  │ " << Color::CYAN << "watch" << Color::RESET
      << "               │ Live auto-updating status view    │\n"
      << "  │ " << Color::CYAN << "config" << Color::RESET
      << "              │ Show current configuration        │\n"
      << "  │ " << Color::CYAN << "clear" << Color::RESET
      << "               │ Clear terminal screen             │\n"
      << "  │ " << Color::CYAN << "help" << Color::RESET
      << "                │ Show this help message            │\n"
      << "  │ " << Color::CYAN << "quit / exit" << Color::RESET
      << "         │ Gracefully shutdown watchdog      │\n"
      << "  └─────────────────────┴───────────────────────────────────┘\n\n";
}

void TerminalRenderer::printWatchHeader(const std::string& timeStr, bool isLive) const {
    if (isLive) {
        std::cout << "\n  " << Color::CYAN << "● LIVE VIEW" << Color::RESET 
                  << "   " << Color::DIM << "Last update: " << timeStr << Color::RESET << "\n"
                  << "  " << Color::DIM << "Press ENTER to return to command input" << Color::RESET << "\n";
    } else {
        std::cout << "\n  " << Color::MAGENTA << "⏸️ SNAPSHOT" << Color::RESET 
                  << "   " << Color::DIM << "Time: " << timeStr << Color::RESET << "\n"
                  << "  " << Color::DIM << "(Historical state prior to change)" << Color::RESET << "\n";
    }
}

void TerminalRenderer::printList(
    const std::vector<std::pair<std::string, bool>> &entries,
    const std::set<std::string>& changedEntities) const {
  std::cout << "\n  ┌────┬────────────────────────────┬──────────┐\n"
            << "  │  # │ Name                       │  Status  │\n"
            << "  ├────┼────────────────────────────┼──────────┤\n";

  int aliveCount = 0;
  int index = 1;
  for (const auto &entry : entries) {
    std::string name = entry.first;
    if (name.length() > 26) {
      name = name.substr(0, 23) + "...";
    }

    std::cout << "  │ " << std::setw(2) << index++ << " │ " << std::left
              << std::setw(26) << name << std::right << " │ ";
    
    if (entry.second) {
      std::cout << Color::GREEN << "● ALIVE " << Color::RESET << " │";
      aliveCount++;
    } else {
      std::cout << Color::RED << "○ DEAD  " << Color::RESET << " │";
    }

    if (changedEntities.count(entry.first) > 0) {
      std::cout << "  " << Color::YELLOW << "⚡ STATE CHANGED" << Color::RESET;
    }
    std::cout << "\n";
  }
  std::cout << "  └────┴────────────────────────────┴──────────┘\n";
  std::cout << "  Monitoring " << entries.size() << " entities (" << aliveCount
            << " alive, " << entries.size() - aliveCount << " dead)\n\n";
}

void TerminalRenderer::printList(
    const std::vector<std::pair<std::string, std::string>> &entries,
    const std::set<std::string>& changedEntities) const {
  std::cout << "\n  ┌────┬────────────────────────────┬──────────┐\n"
            << "  │  # │ Name                       │  Status  │\n"
            << "  ├────┼────────────────────────────┼──────────┤\n";

  int aliveCount = 0;
  int index = 1;
  for (const auto &entry : entries) {
    std::string name = entry.first;
    if (name.length() > 26) {
      name = name.substr(0, 23) + "...";
    }

    std::cout << "  │ " << std::setw(2) << index++ << " │ " << std::left
              << std::setw(26) << name << std::right << " │ ";
    
    const std::string& status = entry.second;
    if (status == "ALIVE") {
      std::cout << Color::GREEN << "● ALIVE " << Color::RESET << " │";
      aliveCount++;
    } else if (status == "STOPPED") {
      std::cout << Color::YELLOW << "⏸ STOP  " << Color::RESET << " │";
    } else if (status == "ZOMBIE") {
      std::cout << Color::MAGENTA << "☠ ZOMBIE" << Color::RESET << " │";
    } else if (status == "STUCK") {
      std::cout << Color::ORANGE << "⚠ STUCK " << Color::RESET << " │";
    } else {
      std::cout << Color::RED << "○ DEAD  " << Color::RESET << " │";
    }

    if (changedEntities.count(entry.first) > 0) {
      std::cout << "  " << Color::YELLOW << "⚡ STATE CHANGED" << Color::RESET;
    }
    std::cout << "\n";
  }
  std::cout << "  └────┴────────────────────────────┴──────────┘\n";
  std::cout << "  Monitoring " << entries.size() << " entities (" << aliveCount
            << " alive, " << entries.size() - aliveCount << " other)\n\n";
}

void TerminalRenderer::printSuccess(const std::string &msg) const {
  std::cout << "  " << Color::GREEN << "✅ " << msg << Color::RESET << "\n\n";
}

void TerminalRenderer::printWarning(const std::string &msg) const {
  std::cout << "  " << Color::YELLOW << "⚠️  " << msg << Color::RESET << "\n\n";
}

void TerminalRenderer::printError(const std::string &msg) const {
  std::cout << "  " << Color::RED << "❌ " << msg << Color::RESET << "\n\n";
}

void TerminalRenderer::printInfo(const std::string &msg) const {
  std::cout << "  " << Color::CYAN << "ℹ️  " << msg << Color::RESET << "\n\n";
}

void TerminalRenderer::printConfig(const std::string &logFile, int heartbeatMs,
                                   int pollMs, int summaryMs) const {
  std::cout << "\n  Current Configuration:\n"
            << "  - Log file: " << Color::CYAN << logFile << Color::RESET
            << "\n"
            << "  - Heartbeat timeout: " << Color::CYAN << heartbeatMs << " ms"
            << Color::RESET << "\n"
            << "  - Poll interval: " << Color::CYAN << pollMs << " ms"
            << Color::RESET << "\n"
            << "  - Summary interval: " << Color::CYAN << summaryMs << " ms"
            << Color::RESET << "\n\n";
}

void TerminalRenderer::clearAll() const { std::cout << "\033[2J\033[3J\033[H"; }

void TerminalRenderer::clearVisible() const { std::cout << "\033[2J\033[H"; }

void TerminalRenderer::moveCursorUp(int lines) const {
    std::cout << "\033[" << lines << "A\r" << std::flush;
}

void TerminalRenderer::clearFromCursor() const { std::cout << "\033[J" << std::flush; }
