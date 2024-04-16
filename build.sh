#!/bin/sh
clang -g -ggdb -O3 -Wall -Wextra -Wno-unused -Wpedantic -Werror *.c ir_ssa/opt/*.c ir_ssa/*.c common/*.c ir_ssa/transform/*.c -o vxcc
