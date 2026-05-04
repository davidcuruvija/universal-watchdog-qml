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
)

target_link_libraries(watchdog_tests
    PRIVATE
        watchdog_core
        Catch2::Catch2WithMain
)

catch_discover_tests(watchdog_tests)