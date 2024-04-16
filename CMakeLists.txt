cmake_minimum_required(VERSION 3.28)
project(vxcc_v3 C)

set(CMAKE_C_STANDARD 99)

add_executable(vxcc_v3 main.c
        ir_ssa/ssa.h
        ir_ssa/ssa.c
        ir_ssa/opt.h
        ir_ssa/opt/loop_simplify.c
        ir_ssa/opt/comparisions.c
        ir_ssa/opt/constant_eval.c
        ir_ssa/opt/inline_vars.c
        ir_ssa/opt/remove_vars.c
        ir_ssa/opt/join_compute.c
        ir_ssa/dump.c
        ir_ssa/verify_ssa.c
        utils.h
        ir_ssa/builder.c
        ir_ssa/eval.c
        common.h
        common/verify.c
        ir_ssa/transform/normalize.c
        ir_ssa/opt/reduce_if.c
        ir_ssa/transform/single_assign.c
        ir_ssa/transform/single_assign_conditional.c
        ir_ssa/opt/reduce_loops.c
        ir_ssa/cir.h
        ir_ssa/verify_cir.c
        ir_ssa/transform/flatten.c
)
