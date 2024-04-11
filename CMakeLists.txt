cmake_minimum_required(VERSION 3.28)
project(vxcc_v3 C)

set(CMAKE_C_STANDARD 99)

add_executable(vxcc_v3 main.c
        ssa.h
        ssa.c
        opt.h
        opt/loop_simplify.c
        opt/comparisions.c
        opt/constant_eval.c
        opt/inline_vars.c
        opt/remove_vars.c
        opt/join_compute.c)
