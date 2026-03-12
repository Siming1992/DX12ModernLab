
file(GLOB_RECURSE RUNTIME_SOURCES "*.cpp" "*.h")

add_library(Runtime ${RUNTIME_SOURCES})
target_include_directories(Runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})