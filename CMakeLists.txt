cmake_minimum_required(VERSION 3.15)
project(main)
find_package(LLVM REQUIRED)
set(CMAKE_CXX_STANDARD 17)
option(LLVM_ENABLE_RTTI ON)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(how_to HowToUseJIT.cpp)
target_link_libraries(how_to LLVM)

add_executable(llvm101 llvm101.cpp)
target_link_libraries(llvm101 LLVM)
