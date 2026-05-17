#include "../include/TerminalRenderer.h"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Color {
constexpr const char *RESET = "\033[0m";
constexpr const char *BOLD = "\033[1m";
constexpr const char *DIM = "\033[2m";
constexpr const char *RED = "\033[31m";
constexpr const char *GREEN = "\033[32m";
constexpr const char *YELLOW = "\033[33m";
constexpr const char *CYAN = "\033[36m";
constexpr const char *MAGENTA = "\033[35m";
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
      << "  ┌──────────────────────────┬───────────────────────────────────┐\n"
      << "  │ " << Color::CYAN << "add process <pid>" << Color::RESET
      << "        │ Start monitoring a process by PID │\n"
      << "  │ " << Color::CYAN << "add thread <name>" << Color::RESET
      << "        │ Start monitoring a named thread   │\n"
      << "  │ " << Color::CYAN << "remove <name>" << Color::RESET
      << "            │ Stop monitoring by name           │\n"
      << "  │ " << Color::CYAN << "kill <name>" << Color::RESET
      << "              │ Send SIGTERM to a monitored process│\n"
      << "  │ " << Color::CYAN << "restart <name>" << Color::RESET
      << "           │ Kill and relaunch a process       │\n"
      << "  │ " << Color::CYAN << "list" << Color::RESET
      << "                     │ Show all monitored entities       │\n"
      << "  │ " << Color::CYAN << "watch" << Color::RESET
      << "                    │ Live auto-updating status view    │\n"
      << "  │ " << Color::CYAN << "sort cpu|memory|name" << Color::RESET
      << "     │ Set sort order for list/watch     │\n"
      << "  │ " << Color::CYAN << "config" << Color::RESET
      << "                   │ Show current configuration        │\n"
      << "  │ " << Color::CYAN << "clear" << Color::RESET
      << "                    │ Clear terminal screen             │\n"
      << "  │ " << Color::CYAN << "help" << Color::RESET
      << "                     │ Show this help message            │\n"
      << "  │ " << Color::CYAN << "quit / exit" << Color::RESET
      << "              │ Gracefully shutdown watchdog      │\n"
      << "  └──────────────────────────┴───────────────────────────────────┘\n\n";
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

static std::string formatMemory(long kb) {
  if (kb <= 0) return "N/A";
  if (kb < 1024) return std::to_string(kb) + " KB";
  double mb = kb / 1024.0;
  if (mb < 1024.0) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << mb << " MB";
    return oss.str();
  }
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << (mb / 1024.0) << " GB";
  return oss.str();
}

static std::string formatCpu(double pct) {
  if (pct <= 0.0) return "N/A";
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1) << pct << "%";
  return oss.str();
}

void TerminalRenderer::printList(
    const std::vector<WatchableStatus> &entries,
    const std::set<std::string>& changedEntities) const {
  std::cout << "\n  ┌────┬────────────────────────────┬──────────┬───────────┬────────┐\n"
            << "  │  # │ Name                       │  Status  │   Memory  │   CPU  │\n"
            << "  ├────┼────────────────────────────┼──────────┼───────────┼────────┤\n";

  int aliveCount = 0;
  int index = 1;
  for (const auto &entry : entries) {
    std::string name = entry.name;
    if (name.length() > 26) {
      name = name.substr(0, 23) + "...";
    }

    std::cout << "  │ " << std::setw(2) << index++ << " │ " << std::left
              << std::setw(26) << name << std::right << " │ ";

    if (entry.alive) {
      std::cout << Color::GREEN << "● ALIVE " << Color::RESET << " │";
      aliveCount++;
    } else {
      std::cout << Color::RED << "○ DEAD  " << Color::RESET << " │";
    }

    std::cout << std::right << std::setw(9) << formatMemory(entry.memoryKB) << "  │";
    std::cout << std::right << std::setw(6) << formatCpu(entry.cpuPercent) << "  │";

    if (changedEntities.count(entry.name) > 0) {
      std::cout << "  " << Color::YELLOW << "⚡ STATE CHANGED" << Color::RESET;
    }
    std::cout << "\n";
  }
  std::cout << "  └────┴────────────────────────────┴──────────┴───────────┴────────┘\n";
  std::cout << "  Monitoring " << entries.size() << " entities (" << aliveCount
            << " alive, " << entries.size() - aliveCount << " dead)\n\n";
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
