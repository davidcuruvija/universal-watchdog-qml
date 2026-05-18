#include "ProcessMonitor.h"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>

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

std::vector<std::string> ProcessMonitor::readCmdline(pid_t pid) {
  std::string path = "/proc/" + std::to_string(pid) + "/cmdline";
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) return {};

  std::vector<std::string> args;
  std::string arg;
  char c;
  while (file.get(c)) {
    if (c == '\0') {
      if (!arg.empty()) {
        args.push_back(std::move(arg));
        arg.clear();
      }
    } else {
      arg += c;
    }
  }
  if (!arg.empty()) args.push_back(std::move(arg));
  return args;
}

ProcessMonitor::ProcessMonitor(pid_t pid)
    : m_pid(pid)
    , m_processName(getProcessName(pid))
    , m_name(m_processName + " (pid=" + std::to_string(pid) + ")")
    , m_cmdline(readCmdline(pid)) {}

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

long ProcessMonitor::readCpuTicks(pid_t pid) {
  std::string path = "/proc/" + std::to_string(pid) + "/stat";
  std::ifstream f(path);
  if (!f.is_open()) return -1;

  std::string line;
  if (!std::getline(f, line)) return -1;

  // Skip past the comm field "(name)" which may contain spaces/parens.
  size_t rpos = line.find_last_of(')');
  if (rpos == std::string::npos || rpos + 2 >= line.size()) return -1;

  // After ") " the fields are: state ppid pgroup session tty_nr tpgid flags
  // minflt cminflt majflt cmajflt utime(11) stime(12)
  std::istringstream iss(line.substr(rpos + 2));
  std::string tok;
  long utime = 0, stime = 0;
  for (int i = 0; i <= 12; ++i) {
    if (!(iss >> tok)) return -1;
    if (i == 11) utime = std::stol(tok);
    if (i == 12) stime = std::stol(tok);
  }
  return utime + stime;
}

double ProcessMonitor::cpuPercent() const {
  long ticks = readCpuTicks(m_pid);
  if (ticks < 0) return 0.0;

  auto now = std::chrono::steady_clock::now();

  if (m_prevCpuTicks < 0) {
    m_prevCpuTicks = ticks;
    m_prevCpuTime = now;
    return 0.0;
  }

  double elapsed = std::chrono::duration<double>(now - m_prevCpuTime).count();
  if (elapsed < 0.01) return 0.0;

  long clkTck = sysconf(_SC_CLK_TCK);
  double pct = 100.0 * static_cast<double>(ticks - m_prevCpuTicks) / (elapsed * clkTck);

  m_prevCpuTicks = ticks;
  m_prevCpuTime = now;

  return (pct < 0.0) ? 0.0 : pct;
}

long ProcessMonitor::memoryKB() const {
  std::string path = "/proc/" + std::to_string(m_pid) + "/status";
  std::ifstream file(path);
  std::string line;
  while (std::getline(file, line)) {
    if (line.compare(0, 6, "VmRSS:") == 0) {
      long kb = 0;
      std::istringstream iss(line.substr(6));
      iss >> kb;
      return kb;
    }
  }
  return 0;
}

bool ProcessMonitor::kill() {
  return ::kill(m_pid, SIGTERM) == 0;
}

bool ProcessMonitor::restart() {
  if (m_cmdline.empty()) return false;

  // Capture the process's working directory before it exits.
  std::string cwdPath = "/proc/" + std::to_string(m_pid) + "/cwd";
  char cwdBuf[4096];
  ssize_t cwdLen = ::readlink(cwdPath.c_str(), cwdBuf, sizeof(cwdBuf) - 1);
  std::string originalCwd;
  if (cwdLen > 0) {
    cwdBuf[cwdLen] = '\0';
    originalCwd = cwdBuf;
  }

  ::kill(m_pid, SIGTERM);

  // Wait up to 1 s for the process to exit after SIGTERM.
  for (int i = 0; i < 10; ++i) {
    usleep(100'000);
    if (!isAlive()) break;
  }

  if (isAlive()) {
    ::kill(m_pid, SIGKILL);
    usleep(100'000);
  }

  std::vector<const char*> argv;
  argv.reserve(m_cmdline.size() + 1);
  for (const auto& arg : m_cmdline) argv.push_back(arg.c_str());
  argv.push_back(nullptr);

  pid_t newPid = fork();
  if (newPid < 0) return false;

  if (newPid == 0) {
    if (!originalCwd.empty()) ::chdir(originalCwd.c_str());
    execvp(argv[0], const_cast<char* const*>(argv.data()));
    _exit(127);
  }

  // Give the child a moment to exec before we start tracking it.
  usleep(50'000);

  m_pid  = newPid;
  m_name = m_processName + " (pid=" + std::to_string(newPid) + ")";
  return true;
}
