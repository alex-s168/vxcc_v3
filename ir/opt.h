#ifndef OPT_H
#define OPT_H

#include "ir.h"

typedef struct {
    size_t max_total_cmov_inline_cost;
    size_t consteval_iterations;
    bool   loop_simplify;
    bool   if_eval;
} vx_OptConfig;

extern vx_OptConfig vx_g_optconfig;

OPT_PASS void vx_opt_loop_simplify(vx_IrBlock *block);
OPT_PASS void vx_opt_comparisions(vx_IrBlock *block);
OPT_PASS void vx_opt_constant_eval(vx_IrBlock *block);
OPT_PASS void vx_opt_inline_vars(vx_IrBlock *block);
OPT_PASS void vx_opt_join_compute(vx_IrBlock *block);
OPT_PASS void vx_opt_reduce_if(vx_IrBlock *block);
OPT_PASS void vx_opt_reduce_loops(vx_IrBlock *block);
OPT_PASS void vx_opt_cmov(vx_IrBlock *block);
OPT_PASS void vx_opt_vars(vx_IrBlock *block);
OPT_PASS void vx_opt_ll_jumps(vx_IrBlock *block);
OPT_PASS void vx_opt_simple_patterns(vx_IrBlock* block);
OPT_PASS void vx_opt_if_opts(vx_IrBlock* block);
OPT_PASS void vx_opt_if_swapCases(vx_IrBlock* block);

OPT_PASS void vx_opt_ll_inlineVars(vx_IrBlock *block);
OPT_PASS void vx_opt_ll_dce(vx_IrBlock *block);
OPT_PASS void vx_opt_ll_condtailcall(vx_IrBlock *block);
OPT_PASS void vx_opt_ll_sched(vx_IrBlock *block);

// TODO: rename to vx_opt

void opt(vx_IrBlock *block);
void opt_preLower(vx_IrBlock *block);
void opt_ll(vx_IrBlock *block);

#endif //OPT_H
