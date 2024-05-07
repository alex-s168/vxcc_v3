cmake_minimum_required(VERSION 3.28)
project(vxcc_v3 C)

set(CMAKE_C_STANDARD 99)

add_executable(vxcc_v3 main.c
        ir/ir.h
        ir/ir.c
        ir/opt.h
        ir/opt/loop_simplify.c
        ir/opt/comparisions.c
        ir/opt/constant_eval.c
        ir/opt/inline_vars.c
        ir/opt/remove_vars.c
        ir/opt/join_compute.c
        ir/dump.c
        ir/verify_ssa.c
        utils.h
        ir/builder.c
        ir/eval.c
        common.h
        common/verify.c
        ir/transform/normalize.c
        ir/opt/reduce_if.c
        ir/transform/single_assign.c
        ir/transform/single_assign_conditional.c
        ir/opt/reduce_loops.c
        ir/cir.h
        ir/verify_cir.c
        ir/transform/flatten.c
        ir/analysis.c
        ir/transform.c
        ir/verify_common.c
        plug/plugin.h
        plug/plugin.c
)
