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
OPT_PASS void vx_opt_tailcall(vx_IrBlock *block);
OPT_PASS void vx_opt_vars(vx_IrBlock *block);
OPT_PASS void vx_opt_ll_jumps(vx_IrBlock *block);

OPT_PASS void vx_opt_ll_dce(vx_IrBlock *block);
OPT_PASS void vx_opt_ll_condtailcall(vx_IrBlock *block);

void opt(vx_IrBlock *block);
void opt_ll(vx_IrBlock *block);

#endif //OPT_H
