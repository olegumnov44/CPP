cmake_minimum_required(VERSION 3.5)

project(searchserver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_BUILD_TYPE "Debug")
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})

add_executable(searchserver main.cpp document.h document.cpp paginator.h read_input_functions.h read_input_functions.cpp
    request_queue.h request_queue.cpp  search_server.h search_server.cpp string_processing.h string_processing.cpp
    log_duration.h test_example_functions.h process_queries.cpp process_queries.h test_framework.h concurrent_map.h
    experimental.h)

#set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH} /usr/include/tbb")
find_package(TBB REQUIRED)
find_package(Threads REQUIRED)
#find_package(TBB REQUIRED tbb)
#find_package(TBB COMPONENTS)

target_link_libraries(searchserver TBB::tbb)
target_link_libraries(searchserver Threads::Threads)
