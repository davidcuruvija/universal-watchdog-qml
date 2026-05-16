#include "ProcessMonitor.h"

#include <cerrno>
#include <csignal>
#include <fstream>
#include <string>
#include <stdexcept>

static std::string getProcessName(pid_t targetPid) {
  std::string path = "/proc/" + std::to_string(targetPid) + "/comm";
  std::ifstream file(path);
  std::string processName;

  if (file.is_open() && std::getline(file, processName) &&
      !processName.empty()) {
    return processName;
  }

  throw std::invalid_argument("Process with given PID does not exist");
}

ProcessMonitor::ProcessMonitor(pid_t pid)
    :m_pid(pid), m_processName(getProcessName(pid)), m_name(m_processName + " (pid=" + std::to_string(pid) + ")") {}

std::string ProcessMonitor::name() const { return m_name; }

char ProcessMonitor::readRawState() const {
  std::ifstream statFile(getStatPath());
  if (!statFile.is_open()) return '\0';

  std::string line;
  if (!std::getline(statFile, line) || line.empty()) return '\0';

  size_t lparenPos = line.find_first_of('(');
  size_t rparenPos = line.find_last_of(')');
  if (lparenPos == std::string::npos || rparenPos == std::string::npos ||
      lparenPos > rparenPos) {
    return '\0';
  }

  std::string processName = line.substr(lparenPos + 1, rparenPos - lparenPos - 1);
  std::string expectedName = m_processName;
  if (expectedName.length() > 15) {
    expectedName = expectedName.substr(0, 15);
  }

  if (processName != expectedName) {
    return '\0';
  }

  if (rparenPos + 2 >= line.length()) {
    return '\0';
  }

  return line[rparenPos + 2];
}

bool ProcessMonitor::isAlive() const {
  char state = readRawState();
  return (state == 'R' || state == 'S' || state == 'D');
}

ProcessState ProcessMonitor::getProcessState() const {
  char state = readRawState();

  switch (state) {
    case 'R':
    case 'S':
      m_inDState = false;
      return ProcessState::ALIVE;

    case 'D': {
      auto now = std::chrono::steady_clock::now();
      if (!m_inDState) {
        m_inDState = true;
        m_dStateSince = now;
        return ProcessState::ALIVE;
      }
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_dStateSince).count();
      if (elapsed >= 30) {
        return ProcessState::STUCK;
      }
      return ProcessState::ALIVE;
    }

    case 'T':
    case 't':
      m_inDState = false;
      return ProcessState::STOPPED;

    case 'Z':
      m_inDState = false;
      return ProcessState::ZOMBIE;

    default:
      m_inDState = false;
      return ProcessState::DEAD;
  }
}
