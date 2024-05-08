#include <stdlib.h>

#include "../opt.h"

void vx_opt_reduce_if(vx_IrView view,
                      vx_IrBlock *block)
{
    const vx_IrBlock *root = vx_IrBlock_root(block);

    while (vx_IrView_find(&view, VX_IR_OP_IF)) {
        vx_IrOp *op = (vx_IrOp *) vx_IrView_take(view);

        vx_IrBlock *cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;
        opt(cond); // !
        const vx_IrVar condVar = cond->outs[0];

        vx_IrBlock *then = vx_IrOp_param(op, VX_IR_NAME_COND_THEN)->block;
        opt(then);

        vx_IrBlock *els = vx_IrOp_param(op, VX_IR_NAME_COND_ELSE)->block;
        opt(els);

        // if it will never be 0 (not might be 0), it is always true => only execute then block
        if (!vx_Irblock_mightbe_var(cond, condVar, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 0 })) {
            for (size_t i = 0; i < op->outs_len; i ++) {
                const vx_IrVar out = op->outs[i].var;
                vx_IrView_rename_var(vx_IrView_of_all(block), block, out, then->outs[i]); // does all the bookkeeping for us
            }

            vx_IrView_replace(block, view, then->ops, then->ops_len);
            free(then->ops);
            view = vx_IrView_drop(view, then->ops_len);
            continue;
        }

        // if it will always we 0, only the else block will ever be executed
        if (vx_Irblock_alwaysis_var(cond, condVar, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 0 })) {
            for (size_t i = 0; i < op->outs_len; i ++) {
                const vx_IrVar out = op->outs[i].var;
                vx_IrView_rename_var(vx_IrView_of_all(block), block, out, els->outs[i]); // does all the bookkeeping for us
            }

            vx_IrView_replace(block, view, els->ops, els->ops_len);
            free(els->ops);
            view = vx_IrView_drop(view, els->ops_len);
            continue;
        }

        for (size_t i = 0; i < op->outs_len; i ++) {
            if (!vx_IrBlock_var_used(root, op->outs[i].var)) {
                vx_IrBlock_remove_out_at(then, i);
                vx_IrBlock_remove_out_at(els, i);
                vx_IrOp_remove_out_at(op, i);
                i --;
            }
        }

        view = vx_IrView_drop(view, 1);
    }
}
