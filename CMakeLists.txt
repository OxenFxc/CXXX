cmake_minimum_required(VERSION 3.10)
project(cxxx VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src/include)
include_directories(src/vm)
include_directories(src/compiler)

# Find all source files (excluding main.cpp)
file(GLOB_RECURSE VM_SOURCES "src/vm/*.cpp")
file(GLOB_RECURSE COMPILER_SOURCES "src/compiler/*.cpp")

# Create library
add_library(libcxxx STATIC ${VM_SOURCES} ${COMPILER_SOURCES})

# Create executable
if(EXISTS "${CMAKE_SOURCE_DIR}/src/cli/main.cpp")
    add_executable(cxxx src/cli/main.cpp)
    target_link_libraries(cxxx PRIVATE libcxxx)
endif()

# Enable testing
enable_testing()
add_subdirectory(tests)
