cmake_minimum_required(VERSION 3.16)
project(pankha VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt 6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# Source files
set(SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
    FanApiClient.cpp
    FanApiClient.h
    BackendLauncher.cpp
    BackendLauncher.h
    resources.qrc
    resource.rc
)

add_executable(FanControlHost ${SOURCES})

target_link_libraries(FanControlHost PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Set subsystem as Windows GUI application (no console window)
set_target_properties(FanControlHost PROPERTIES
    WIN32_EXECUTABLE ON
)
