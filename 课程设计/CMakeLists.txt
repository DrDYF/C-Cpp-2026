cmake_minimum_required(VERSION 3.21)
project(gobang CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# raylib 编译选项
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/raylib)

# 收集所有源文件
file(GLOB_RECURSE SOURCES 
    src/*.cpp
    src/core/*.cpp
    src/ai/*.cpp
    src/ui/*.cpp
)

add_executable(gobang ${SOURCES})

target_include_directories(gobang PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(gobang PRIVATE raylib)

# Windows 下使用 GUI 模式，不弹出控制台
if(WIN32)
    set_target_properties(gobang PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
