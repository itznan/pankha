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
    src/frontend/main.cpp
    src/frontend/MainWindow.cpp
    src/frontend/MainWindow_UI.cpp
    src/frontend/MainWindow_Slots.cpp
    src/frontend/MainWindow_Settings.cpp
    src/frontend/MainWindow.h
    src/frontend/FanApiClient.cpp
    src/frontend/FanApiClient.h
    src/frontend/BackendLauncher.cpp
    src/frontend/BackendLauncher.h
    src/frontend/components/SmoothScrollFilter.h
    src/frontend/components/SmoothScrollFilter.cpp
    src/frontend/components/FanCardWidget.h
    src/frontend/components/FanCardWidget.cpp
    src/frontend/resources.qrc
    src/frontend/resource.rc
)

add_executable(FanControlHost ${SOURCES})

target_link_libraries(FanControlHost PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    dwmapi
)

# Set subsystem as Windows GUI application (no console window)
set_target_properties(FanControlHost PROPERTIES
    WIN32_EXECUTABLE ON
)
