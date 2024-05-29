#include "../opt.h"

void vx_opt_reduce_if(vx_IrBlock *block)
{
    vx_IrBlock *root = vx_IrBlock_root(block);

    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->id != VX_IR_OP_IF)
            continue;

        vx_IrBlock *cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;
        const vx_IrVar condVar = cond->outs[0];

        vx_IrValue *pthen = vx_IrOp_param(op, VX_IR_NAME_COND_THEN);
        vx_IrBlock *then = pthen ? pthen->block : NULL;

        vx_IrValue *pelse = vx_IrOp_param(op, VX_IR_NAME_COND_ELSE);
        vx_IrBlock *els = pelse ? pelse->block : NULL;

        if (vx_IrBlock_empty(then) && vx_IrBlock_empty(els)) {
            vx_IrOp_remove(op);
            continue;
        }

        if (then) {
            // if it will never be 0 (not might be 0), it is always true => only execute then block
            if (!vx_Irblock_mightbe_var(cond, condVar, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 0 })) {
                for (size_t i = 0; i < op->outs_len; i ++) {
                    const vx_IrVar out = op->outs[i].var;
                    vx_IrBlock_rename_var(block, out, then->outs[i]); // does all the bookkeeping for us
                }

                op->id = VX_IR_OP_FLATTEN_PLEASE;
                vx_IrOp_add_param_s(op, VX_IR_NAME_BLOCK, *pthen);
                continue;
            }
        }

        if (els) {
            // if it will always we 0, only the else block will ever be executed
            if (vx_Irblock_alwaysis_var(cond, condVar, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 0 })) {
                for (size_t i = 0; i < op->outs_len; i ++) {
                    const vx_IrVar out = op->outs[i].var;
                    vx_IrBlock_rename_var(block, out, els->outs[i]); // does all the bookkeeping for us
                }

                op->id = VX_IR_OP_FLATTEN_PLEASE;
                vx_IrOp_add_param_s(op, VX_IR_NAME_BLOCK, *pelse);
                continue;
            }
        }

        for (size_t i = 0; i < op->outs_len; i ++) {
            if (!vx_IrBlock_var_used(root, op->outs[i].var)) {
                if (pthen)
                    vx_IrBlock_remove_out_at(pthen->block, i);
                if (pelse)
                    vx_IrBlock_remove_out_at(pelse->block, i);
                vx_IrOp_remove_out_at(op, i);
                i --;
            }
        }
    }
}
