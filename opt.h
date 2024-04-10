#ifndef OPT_H
#define OPT_H

#include "ssa.h"

void opt_loop_simplify(SsaView view, SsaBlock *block);
void opt_comparisions(SsaView view, SsaBlock *block);
void opt_constant_eval(SsaView view, SsaBlock *block);
void opt_inline_vars(SsaView view, SsaBlock *block);
void opt_remove_vars(SsaView view, SsaBlock *block);

static void opt(SsaBlock *block) {
    opt_constant_eval(ssaview_of_all(block), block);
    opt_inline_vars(ssaview_of_all(block), block);
    opt_remove_vars(ssaview_of_all(block), block);
    opt_comparisions(ssaview_of_all(block), block);
    opt_loop_simplify(ssaview_of_all(block), block);
}

#endif //OPT_H
