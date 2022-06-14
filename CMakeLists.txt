cmake_minimum_required(VERSION 3.16)
project(magpie CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Define BUILD_VERSION from Git tag
EXECUTE_PROCESS(
    COMMAND git describe --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    TIMEOUT 5
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
MESSAGE(STATUS "Building from Git tag ${GIT_VERSION}")
add_definitions(-DBUILD_VERSION=\"${GIT_VERSION}\")

# Use libc++ if available
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_library(LIBCPP c++)
    if (LIBCPP)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
endif()

# C++ Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

# libtins
option(LIBTINS_BUILD_EXAMPLES "" OFF)
option(LIBTINS_BUILD_TESTS "" OFF)
option(LIBTINS_BUILD_SHARED "" OFF)
option(LIBTINS_ENABLE_CXX11 "" ON)
option(LIBTINS_ENABLE_DOT11 "" OFF)
option(LIBTINS_ENABLE_TCPIP "" OFF)
option(LIBTINS_ENABLE_ACK_TRACKER "" OFF)
option(LIBTINS_ENABLE_TCP_STREAM_CUSTOM_DATA "" OFF)
add_subdirectory(vendor/libtins)
include_directories(vendor/libtins/include)

# fmt
set(FMT_DEBUG_POSTFIX "" CACHE STRING "")
add_subdirectory(vendor/fmt)
include_directories(vendor/fmt/include)

# cereal
include_directories(vendor/cereal/include)

add_subdirectory(src)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
