cmake_minimum_required(VERSION 3.20)

project(TacticalMessageSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Reusable C++ messaging backend.
add_library(messaging_core
        src/Message.cpp
        src/QueueNode.cpp
        src/StackNode.cpp
        src/PriorityScheduler.cpp
        src/MessageProcessor.cpp
)

# Make headers in include/ available to the library and its consumers.
target_include_directories(messaging_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Normal application executable.
add_executable(TacticalMessageSimulator
        apps/main.cpp
)
target_link_libraries(TacticalMessageSimulator PRIVATE messaging_core)

# Independent performance benchmark executable.
add_executable(scheduler_benchmark
        benchmarks/scheduler_benchmark.cpp
)
target_link_libraries(scheduler_benchmark PRIVATE messaging_core)

# Add test executables and CTest registrations when tests are implemented.

# Standalone automated correctness tests.
enable_testing()
add_executable(test_scheduler tests/test_scheduler.cpp)
target_link_libraries(test_scheduler PRIVATE messaging_core)
add_test(NAME SchedulerTests COMMAND test_scheduler)
