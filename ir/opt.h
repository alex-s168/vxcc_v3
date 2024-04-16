#ifndef OPT_H
#define OPT_H

#include "ir.h"

void opt_loop_simplify(SsaView view, SsaBlock *block);
void opt_comparisions(SsaView view, SsaBlock *block);
void opt_constant_eval(SsaView view, SsaBlock *block);
void opt_inline_vars(SsaView view, SsaBlock *block);
void opt_join_compute(SsaView view, SsaBlock *block);
void opt_reduce_if(SsaView view, SsaBlock *block);
void opt_reduce_loops(SsaView view, SsaBlock *block);

// CALL ONLY ON ROOT BLOCKS:
void opt_remove_vars(SsaBlock *block);

#define CONSTEVAL_ITER 6

static void opt_pre(SsaBlock *block) {
    // place immediates into params
    opt_inline_vars(irview_of_all(block), block);
    for (int i = 0; i < CONSTEVAL_ITER; i ++) {
        // evaluate constants
        opt_constant_eval(irview_of_all(block), block);
        // place results into params
        opt_inline_vars(irview_of_all(block), block);
    }

    if (block->is_root)
        opt_remove_vars(block);

    opt_comparisions(irview_of_all(block), block);
}

static void opt(SsaBlock *block) {
    opt_pre(block);
    opt_join_compute(irview_of_all(block), block);
    opt_reduce_if(irview_of_all(block), block);
    opt_pre(block);
    opt_reduce_loops(irview_of_all(block), block);
    opt_loop_simplify(irview_of_all(block), block);
}

#endif //OPT_H