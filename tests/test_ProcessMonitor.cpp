#include <catch2/catch_all.hpp>

#include "ProcessMonitor.h"

#include <csignal>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

TEST_CASE("ProcessMonitor tracks current process lifecycle", "[ProcessMonitor]") {
  auto monitor = std::make_shared<ProcessMonitor>(getpid());
  REQUIRE(monitor->isAlive() == true);
}

TEST_CASE("ProcessMonitor tracks a process that terminates naturally", "[ProcessMonitor]") {
  // 1. Start a process that only lives for 1 second
  FILE *pipe = popen("sleep 1 & echo $!", "r");
  char buffer[16];
  fgets(buffer, sizeof(buffer), pipe);
  pid_t externalPid = std::stoi(buffer);
  pclose(pipe);

  // Give it a fraction of a second to exec() so the name changes from "sh" to
  // "sleep"
  usleep(100000);

  auto monitor = std::make_shared<ProcessMonitor>(externalPid);

  // 2. Verify it is alive at the start AND check the name
  REQUIRE(monitor->isAlive() == true);
  REQUIRE(monitor->name() == "sleep (pid=" + std::to_string(externalPid) + ")");

  // 3. Wait for 1.5 seconds so the process has time to finish naturally
  usleep(1500000);

  // 4. Verify that it is now DEAD without us killing it manually
  REQUIRE(monitor->isAlive() == false);
}

TEST_CASE("ProcessMonitor tracks system init process (PID 1)", "[ProcessMonitor]") {
  // PID 1 is the init process (systemd/init) and should always be alive
  auto monitor = std::make_shared<ProcessMonitor>(1);
  REQUIRE(monitor->isAlive() == true);

  // For the tested Ubuntu Linux system, PID 1 name is systemd
  REQUIRE(monitor->name() == "systemd (pid=1)");
}

TEST_CASE("ProcessMonitor throws on invalid/non-existent process", "[ProcessMonitor]") {
  // 9999999 is well above the maximum PID limit on standard Linux systems
  REQUIRE_THROWS_AS(std::make_shared<ProcessMonitor>(9999999), std::invalid_argument);
}

TEST_CASE("ProcessMonitor tracks zombie process", "[ProcessMonitor]") {
  pid_t pid = fork();
  REQUIRE(pid >= 0);

  if (pid == 0) {
    // Child process exits immediately to become a zombie
    std::exit(0);
  } else {
    // Parent process
    // Give the child a moment to exit and become a zombie
    usleep(50000);

    auto monitor = std::make_shared<ProcessMonitor>(pid);

    // Zombie processes should be reported as dead (false)
    REQUIRE(monitor->isAlive() == false);

    // Clean up the zombie process to avoid polluting the OS
    int status;
    waitpid(pid, &status, 0);
  }
}

TEST_CASE("ProcessMonitor tracks stopped/frozen process (T state)", "[ProcessMonitor]") {
  pid_t pid = fork();
  REQUIRE(pid >= 0);

  if (pid == 0) {
    // Child process loops infinitely until killed
    while (true) {
      usleep(10000);
    }
  } else {
    // Parent process
    usleep(50000); // Give the child time to fully start

    auto monitor = std::make_shared<ProcessMonitor>(pid);

    // Initially, the process is running normally
    REQUIRE(monitor->isAlive() == true);

    // Send SIGSTOP to freeze the process (puts it in 'T' state)
    kill(pid, SIGSTOP);
    usleep(50000); // Give OS time to update the state in /proc

    // Our monitor should now report it as dead
    REQUIRE(monitor->isAlive() == false);

    // Clean up: brutally kill the stopped process and wait for it
    kill(pid, SIGKILL);
    int status;
    waitpid(pid, &status, 0);
  }
}

TEST_CASE("ProcessMonitor returns getProcessState correctly", "[ProcessMonitor][State]") {
  SECTION("Running process returns ALIVE state") {
    auto monitor = std::make_shared<ProcessMonitor>(getpid());
    REQUIRE(monitor->getProcessState() == ProcessState::ALIVE);
  }

  SECTION("Stopped process returns STOPPED state") {
    pid_t pid = fork();
    REQUIRE(pid >= 0);
    if (pid == 0) {
      while (true) { usleep(10000); }
    } else {
      usleep(50000);
      auto monitor = std::make_shared<ProcessMonitor>(pid);
      REQUIRE(monitor->getProcessState() == ProcessState::ALIVE);
      
      kill(pid, SIGSTOP);
      usleep(50000);
      REQUIRE(monitor->getProcessState() == ProcessState::STOPPED);
      
      kill(pid, SIGKILL);
      int status;
      waitpid(pid, &status, 0);
    }
  }

  SECTION("Zombie process returns ZOMBIE state") {
    pid_t pid = fork();
    REQUIRE(pid >= 0);
    if (pid == 0) {
      std::exit(0);
    } else {
      usleep(50000);
      auto monitor = std::make_shared<ProcessMonitor>(pid);
      REQUIRE(monitor->getProcessState() == ProcessState::ZOMBIE);
      
      int status;
      waitpid(pid, &status, 0);
    }
  }

  SECTION("Non-existent process returns DEAD state") {
    // A process that dies after being successfully created
    pid_t pid = fork();
    REQUIRE(pid >= 0);
    if (pid == 0) {
      std::exit(0);
    } else {
      usleep(50000);
      auto monitor = std::make_shared<ProcessMonitor>(pid);
      int status;
      waitpid(pid, &status, 0); // Reap the zombie, now it is fully dead
      REQUIRE(monitor->getProcessState() == ProcessState::DEAD);
    }
  }
}
