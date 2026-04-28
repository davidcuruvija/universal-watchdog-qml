#include "ProcessMonitor.h"

#include <cerrno>
#include <csignal>
#include <fstream>
#include <string>

static std::string getProcessName(pid_t targetPid) {
  std::string path = "/proc/" + std::to_string(targetPid) + "/comm";
  std::ifstream file(path);
  std::string processName;

  if (file.is_open() && std::getline(file, processName) &&
      !processName.empty()) {
    return processName;
  }

  return "UnknownProcess";
}

ProcessMonitor::ProcessMonitor(pid_t pid)
    : m_name(getProcessName(pid)), m_pid(pid) {}

std::string ProcessMonitor::name() const { return m_name; }

bool ProcessMonitor::isAlive() const {
  std::string statPath = getStatPath();
  std::ifstream statFile(statPath);
  if (!statFile.is_open()) {
    return false;
  }
  std::string line;
  if (!std::getline(statFile, line) || line.empty()) {
    return false;
  }
  size_t lparenPos = line.find_first_of('(');
  size_t rparenPos = line.find_last_of(')');

  if (lparenPos == std::string::npos || rparenPos == std::string::npos ||
      lparenPos > rparenPos) {
    return false;
  }

  std::string processName =
      line.substr(lparenPos + 1, rparenPos - lparenPos - 1);
  std::string expectedName = m_name;
  if (expectedName.length() > 15) {
    expectedName = expectedName.substr(0, 15);
  }

  if (processName != expectedName) {
    return false;
  }

  if (rparenPos + 2 >= line.length()) {
    return false;
  }

  char state = line[rparenPos + 2];
  if (state == 'R' || state == 'S' || state == 'D') {
    return true;
  }

  return false;
}
