#!/bin/sh
clang -g -ggdb -O3 -Wall -Wextra -Wno-unused -Wpedantic -Werror *.c opt/*.c -o main
