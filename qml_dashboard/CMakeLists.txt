find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup()

qt_add_executable(watchdog_dashboard
    main.cpp
    WatchdogModel.cpp
)

qt_add_qml_module(watchdog_dashboard
    URI WatchdogDashboard
    VERSION 1.0
    QML_FILES
        qml/Dashboard.qml
)

target_link_libraries(watchdog_dashboard
    PRIVATE
        watchdog_core
        Qt6::Quick
)
