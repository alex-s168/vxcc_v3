#include <stdlib.h>

#include "../opt.h"

void opt_reduce_if(SsaView view, SsaBlock *block) {
    const SsaBlock *root = irblock_root(block);

    while (irview_find(&view, SSA_OP_IF)) {
        SsaOp *op = (SsaOp *) irview_take(view);

        SsaBlock *cond = irop_param(op, SSA_NAME_COND)->block;
        opt(cond); // !
        const SsaVar condVar = cond->outs[0];

        SsaBlock *then = irop_param(op, SSA_NAME_COND_THEN)->block;
        opt(then);

        SsaBlock *els = irop_param(op, SSA_NAME_COND_ELSE)->block;
        opt(els);

        // if it will never be 0 (not might be 0), it is always true => only execute then block
        if (!irblock_mightbe_var(cond, condVar, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 })) {
            for (size_t i = 0; i < op->outs_len; i ++) {
                const SsaVar out = op->outs[i].var;
                irview_rename_var(irview_of_all(block), block, out, then->outs[i]); // does all the bookkeeping for us
            }

            irview_replace(block, view, then->ops, then->ops_len);
            free(then->ops);
            view = irview_drop(view, then->ops_len);
            continue;
        }

        // if it will always we 0, only the else block will ever be executed
        if (irblock_alwaysis_var(cond, condVar, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 })) {
            for (size_t i = 0; i < op->outs_len; i ++) {
                const SsaVar out = op->outs[i].var;
                irview_rename_var(irview_of_all(block), block, out, els->outs[i]); // does all the bookkeeping for us
            }

            irview_replace(block, view, els->ops, els->ops_len);
            free(els->ops);
            view = irview_drop(view, els->ops_len);
            continue;
        }

        for (size_t i = 0; i < op->outs_len; i ++) {
            if (!irblock_var_used(root, op->outs[i].var)) {
                irblock_remove_out_at(then, i);
                irblock_remove_out_at(els, i);
                irop_remove_out_at(op, i);
                i --;
            }
        }

        view = irview_drop(view, 1);
    }
}
