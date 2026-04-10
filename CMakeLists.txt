cmake_minimum_required(VERSION 3.16)

project(TaskScheduler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
enable_testing()

option(TASK_SCHEDULER_ENABLE_TESTS "Build unit and integration tests" ON)
option(TASK_SCHEDULER_FETCH_GTEST "Fetch GoogleTest if it is not already installed" OFF)

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

if(TASK_SCHEDULER_ENABLE_TESTS)
    find_package(GTest CONFIG QUIET)

    if(NOT GTest_FOUND AND TASK_SCHEDULER_FETCH_GTEST)
        include(FetchContent)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(googletest)
    endif()

    find_package(GTest CONFIG QUIET)

    if(NOT GTest_FOUND)
        message(WARNING "GoogleTest was not found. Tests will not be built. Install GTest or configure with TASK_SCHEDULER_FETCH_GTEST=ON in a network-enabled environment.")
    else()
        include(GoogleTest)

        add_executable(task_scheduler_unit_tests
            tests/unit/job_test.cpp
            tests/unit/scheduler_test.cpp
        )

        add_executable(task_scheduler_integration_tests
            tests/integration/scheduler_integration_test.cpp
        )

        target_link_libraries(task_scheduler_unit_tests
            PRIVATE
                task_scheduler
                Threads::Threads
                GTest::gtest_main
                GTest::gmock
        )

        target_link_libraries(task_scheduler_integration_tests
            PRIVATE
                task_scheduler
                Threads::Threads
                GTest::gtest_main
                GTest::gmock
        )

        gtest_discover_tests(task_scheduler_unit_tests)
        gtest_discover_tests(task_scheduler_integration_tests)
    endif()
endif()
