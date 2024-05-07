#ifndef OPT_H
#define OPT_H

#include "ir.h"

void vx_opt_loop_simplify(vx_IrView view, vx_IrBlock *block);
void vx_opt_comparisions(vx_IrView view, vx_IrBlock *block);
void vx_opt_constant_eval(vx_IrView view, vx_IrBlock *block);
void vx_opt_inline_vars(vx_IrView view, vx_IrBlock *block);
void vx_opt_join_compute(vx_IrView view, vx_IrBlock *block);
void vx_opt_reduce_if(vx_IrView view, vx_IrBlock *block);
void vx_opt_reduce_loops(vx_IrView view, vx_IrBlock *block);

/** Can only be applied to root blocks */
void vx_opt_remove_vars(vx_IrBlock *block);

#define CONSTEVAL_ITER 6

static void opt_pre(vx_IrBlock *block) {
    // place immediates into params
    vx_opt_inline_vars(vx_IrView_of_all(block), block);
    for (int i = 0; i < CONSTEVAL_ITER; i ++) {
        // evaluate constants
        vx_opt_constant_eval(vx_IrView_of_all(block), block);
        // place results into params
        vx_opt_inline_vars(vx_IrView_of_all(block), block);
    }

    if (block->is_root)
        vx_opt_remove_vars(block);

    vx_opt_comparisions(vx_IrView_of_all(block), block);
}

static void opt(vx_IrBlock *block) {
    opt_pre(block);
    vx_opt_join_compute(vx_IrView_of_all(block), block);
    vx_opt_reduce_if(vx_IrView_of_all(block), block);
    opt_pre(block);
    vx_opt_reduce_loops(vx_IrView_of_all(block), block);
    vx_opt_loop_simplify(vx_IrView_of_all(block), block);
}

#endif //OPT_H
