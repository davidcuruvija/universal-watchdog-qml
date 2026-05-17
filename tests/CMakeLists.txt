include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)

FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

add_executable(watchdog_tests
    test_logger.cpp
    test_ThreadMonitor.cpp
    test_watchdog.cpp
    test_ProcessMonitor.cpp
    test_CommandParser.cpp
)

target_link_libraries(watchdog_tests
    PRIVATE
        watchdog_core
        Catch2::Catch2WithMain
)

catch_discover_tests(watchdog_tests)

# Manual testing utilities
add_executable(make_zombie manual/make_zombie.cpp)
add_executable(make_stuck manual/make_stuck.cpp)