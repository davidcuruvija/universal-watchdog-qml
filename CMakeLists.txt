cmake_minimum_required(VERSION 3.16)
project(watchdog
    VERSION 0.1.0
    DESCRIPTION "Universal Watchdog"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(WATCHDOG_BUILD_TESTS "Build unit tests" ON)

# -----------------------------------------------------------------------------
# Watchdog core library
# -----------------------------------------------------------------------------
add_library(watchdog_core
    src/logger.cpp
    src/ThreadMonitor.cpp
    src/watchdog.cpp
    src/ProcessMonitor.cpp
    src/CommandParser.cpp
    src/TerminalRenderer.cpp
)

target_include_directories(watchdog_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Threads za std::mutex, std::thread
find_package(Threads REQUIRED)
target_link_libraries(watchdog_core PUBLIC Threads::Threads)


# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
if(WATCHDOG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# -----------------------------------------------------------------------------
# Watchdog application (executable)
# -----------------------------------------------------------------------------
add_executable(watchdog_app src/main.cpp)
target_link_libraries(watchdog_app PRIVATE watchdog_core)
