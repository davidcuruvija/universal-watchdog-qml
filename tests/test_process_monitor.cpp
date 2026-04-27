#include <catch2/catch_all.hpp>

#include "ProcessMonitor.h"

#include <csignal>
#include <cstdlib>
#include <memory>
#include <sys/wait.h>
#include <unistd.h>

TEST_CASE("ProcessMonitor tracks process lifecycle", "[ProcessMonitor]") {
  SECTION("Monitor current process") {
    auto monitor = std::make_shared<ProcessMonitor>(getpid());
    REQUIRE(monitor->isAlive() == true);
  }

  SECTION("Monitor a process that terminates naturally") {
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
    REQUIRE(monitor->name() == "sleep");

    // 3. Wait for 1.5 seconds so the process has time to finish naturally
    usleep(1500000);

    // 4. Verify that it is now DEAD without us killing it manually
    REQUIRE(monitor->isAlive() == false);
  }

  SECTION("Monitor system init process (PID 1)") {
    // PID 1 is the init process (systemd/init) and should always be alive
    auto monitor = std::make_shared<ProcessMonitor>(1);
    REQUIRE(monitor->isAlive() == true);

    // For the tested Ubuntu Linux system, PID 1 name is systemd
    REQUIRE(monitor->name() == "systemd");
  }

  SECTION("Monitor invalid/non-existent process") {
    // 9999999 is well above the maximum PID limit on standard Linux systems
    auto monitor = std::make_shared<ProcessMonitor>(9999999);

    // It should definitely not be alive
    REQUIRE(monitor->isAlive() == false);

    // It shouldn't be able to read a name from /proc
    REQUIRE(monitor->name() == "UnknownProcess");
  }

  SECTION("Monitor zombie process") {
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
}
