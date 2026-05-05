# C++ Universal Watchdog

A lightweight watchdog system written in modern C++ that monitors processes and threads,
detects failures (crashes, freezes), and logs alerts.

## Features
- Register and monitor processes (via PID)
- Monitor threads using heartbeat mechanism
- Periodic health checks
- Logging to console and file
- Modular and extensible design

## Architecture
The system is built around three core components:

- **Watchdog Engine**
  - Periodically checks registered components

- **Watchable Interface**
  - Abstract interface for anything that can be monitored

- **Monitors**
  - ProcessMonitor (PID-based)
  - ThreadMonitor (heartbeat-based)


## Build Instructions

### Requirements
- C++17 or newer
- Linux
- g++

### Build

```bash
mkdir -p build
cd build
cmake ..
make
```

### Running Tests

```bash
./build/tests/watchdog_tests
```

## Limitations
- Linux-only
- No automatic process restart
- No distributed monitoring
- Polling-based (not event-driven)
