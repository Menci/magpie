file(GLOB SRC "*.cc" "**/*.cc" "common/**/*.cc")
add_executable(magpie ${SRC})
include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/src/common
)

target_link_libraries(
    magpie
    ${CMAKE_BINARY_DIR}/vendor/libtins/lib/libtins.a
    pcap
    ${CMAKE_BINARY_DIR}/vendor/fmt/libfmtd.a
)
add_dependencies(magpie tins fmt)
