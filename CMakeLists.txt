cmake_minimum_required(VERSION 3.28)
project(vxcc_v3 C)

set(CMAKE_C_STANDARD 99)

add_executable(vxcc_v3 main.c
        ssa.h
        ssa.c
        opt.h
        opt/loop_simplify.c)
