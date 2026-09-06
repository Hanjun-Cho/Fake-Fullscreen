cmake_minimum_required(VERSION 3.15)
project(fakefullscreen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(fakefullscreen WIN32
    src/main.cpp
    src/winutil.cpp
    src/Actions.cpp
    src/Config.cpp
    src/HotkeyManager.cpp
)

target_include_directories(fakefullscreen PRIVATE src)
target_link_libraries(fakefullscreen PRIVATE user32 shell32)
