
project(rsdco)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE true)

add_subdirectory(${PROJECT_SOURCE_DIR}/hartebeest)

include_directories(${PROJECT_SOURCE_DIR}/hartebeest)
link_directories(${PROJECT_SOURCE_DIR}/hartebeest/build/lib)

set(rsdco_lib      rsdco)
file(GLOB rsdco_lib_src 
    CONFIGURE_DEPENDS 
    ${PROJECT_SOURCE_DIR}/src/*.c
    ${PROJECT_SOURCE_DIR}/src/*.cc 
    ${PROJECT_SOURCE_DIR}/includes/*.h
    ${PROJECT_SOURCE_DIR}/includes/*.hh
    )

add_library(${rsdco_lib} SHARED ${rsdco_lib_src})
set_target_properties(
    ${rsdco_lib} 
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib
)
target_link_libraries(${rsdco_lib} PUBLIC ibverbs pthread memcached hartebeest)
# link_directories(${PROJECT_SOURCE_DIR}/build/lib)

# Test Binaries
message("-- rsdco Test Binaries --")
set(rsdco_demo     rsdco-demo)
add_executable(
    ${rsdco_demo}
    ${PROJECT_SOURCE_DIR}/src/tests/demo.c
)
set_target_properties(
    ${rsdco_demo} 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/bin
)
target_link_libraries(${rsdco_demo} PUBLIC ${rsdco_lib} hartebeest)
