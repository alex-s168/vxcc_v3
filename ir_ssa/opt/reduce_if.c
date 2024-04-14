#include <stdlib.h>

#include "../opt.h"

void opt_reduce_if(SsaView view, SsaBlock *block) {
    const SsaBlock *root = ssablock_root(block);

    while (ssaview_find(&view, SSA_OP_IF)) {
        SsaOp *op = (SsaOp *) ssaview_take(view);

        SsaBlock *cond = ssaop_param(op, "cond")->block;
        opt(cond); // !
        const SsaVar condVar = cond->outs[0];

        SsaBlock *then = ssaop_param(op, "then")->block;
        opt(then);

        SsaBlock *els = ssaop_param(op, "else")->block;
        opt(els);

        // if it will never be 0 (not might be 0), it is always true => only execute then block
        if (!ssablock_mightbe_var(cond, condVar, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 })) {
            for (size_t i = 0; i < op->outs_len; i ++) {
                const SsaVar out = op->outs[i].var;
                ssaview_rename_var(ssaview_of_all(block), block, out, then->outs[i]); // does all the bookkeeping for us
            }

            ssaview_replace(block, view, then->ops, then->ops_len);
            free(then->ops);
            view = ssaview_drop(view, then->ops_len);
            continue;
        }

        // if it will always we 0, only the else block will ever be executed
        if (ssablock_alwaysis_var(cond, condVar, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 })) {
            for (size_t i = 0; i < op->outs_len; i ++) {
                const SsaVar out = op->outs[i].var;
                ssaview_rename_var(ssaview_of_all(block), block, out, els->outs[i]); // does all the bookkeeping for us
            }

            ssaview_replace(block, view, els->ops, els->ops_len);
            free(els->ops);
            view = ssaview_drop(view, els->ops_len);
            continue;
        }

        for (size_t i = 0; i < op->outs_len; i ++) {
            if (!ssablock_var_used(root, op->outs[i].var)) {
                ssablock_remove_out(then, i);
                ssablock_remove_out(els, i);
                ssaop_remove_out(op, i);
                i --;
            }
        }

        view = ssaview_drop(view, 1);
    }
}
