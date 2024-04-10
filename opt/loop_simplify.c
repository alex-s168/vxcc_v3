#include "../opt.h"

void opt_loop_simplify(SsaView view, SsaBlock *block) {
    while (ssaview_find(&view, SSA_OP_FOR)) {
        const SsaOp *op = ssaview_take(view);
        view = ssaview_drop(view, 1);

        SsaBlock *cond = ssaop_param(op, "cond")->block;
        opt(cond);

        
    }
}