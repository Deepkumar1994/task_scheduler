cmake_minimum_required(VERSION 3.16)

project(TaskScheduler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(task_scheduler
    src/job.cpp
    src/scheduler.cpp
)

target_include_directories(task_scheduler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(task_scheduler_demo
    src/main.cpp
)

target_link_libraries(task_scheduler_demo
    PRIVATE
        task_scheduler
        Threads::Threads
)

target_link_libraries(task_scheduler
    PUBLIC
        Threads::Threads
)
