cmake_minimum_required(VERSION 3.28)
project(vxcc_v3 C)

set(CMAKE_C_STANDARD 99)

file(GLOB source test.c ir/opt/*.c ir/*.c common/*.c ir/transform/*.c cg/x86_stupid/*.c)
add_executable(vxcc_v3 ${source})
