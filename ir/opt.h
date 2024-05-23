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

OPT_PASS void vx_opt_loop_simplify(vx_IrView view, vx_IrBlock *block);
OPT_PASS void vx_opt_comparisions(vx_IrView view, vx_IrBlock *block);
OPT_PASS void vx_opt_constant_eval(vx_IrView view, vx_IrBlock *block);
OPT_PASS void vx_opt_inline_vars(vx_IrView view, vx_IrBlock *block);
OPT_PASS void vx_opt_join_compute(vx_IrView view, vx_IrBlock *block);
OPT_PASS void vx_opt_reduce_if(vx_IrView view, vx_IrBlock *block);
OPT_PASS void vx_opt_reduce_loops(vx_IrView view, vx_IrBlock *block);
OPT_PASS void vx_opt_cmov(vx_IrView view, vx_IrBlock *block);

OPT_PASS void vx_opt_ll_jumps(vx_IrView view, vx_IrBlock *block);

/** Can only be applied to root blocks */
OPT_PASS void vx_opt_remove_vars(vx_IrBlock *block);

void opt(vx_IrBlock *block);
void opt_ll(vx_IrBlock *block);

#endif //OPT_H
