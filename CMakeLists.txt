cmake_minimum_required(VERSION 3.15)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(BUILD_MODE Debug)
SET(CMAKE_CXX_FLAGS "-Wall")
SET(CMAKE_CXX_FLAGS_DEBUG "-g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(DB_Engine)

add_executable(${PROJECT_NAME} main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/DiskManager/disk_manager.cpp  ${CMAKE_CURRENT_SOURCE_DIR}/BufferPoolManager/buffer_bool_manager.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/DiskManager   ${CMAKE_CURRENT_SOURCE_DIR}/BufferPoolManager)