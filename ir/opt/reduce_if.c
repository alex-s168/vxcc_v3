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

        if (vx_IrBlock_empty(then) && vx_IrBlock_empty(els) && !then->outs_len && !els->outs_len) {
            vx_IrOp_remove(op);
            continue;
        }

        if (then) {
            // if it will never be 0 (not might be 0), it is always true => only execute then block
            if (!vx_Irblock_mightbeVar(cond, condVar, VX_IR_VALUE_IMM_INT(0))) {
                for (size_t i = 0; i < op->outs_len; i ++) {
                    const vx_IrVar out = op->outs[i].var;
                    vx_IrBlock_renameVar(block, out, then->outs[i], VX_RENAME_VAR_BOTH); // does all the bookkeeping for us
                }

                vx_IrBlock_insertAllOpAfter(block, pthen->block, op);
                vx_IrOp_removeParam(op, VX_IR_NAME_COND_THEN); // don't want to delete that block
                vx_IrOp_remove(op);
                continue;
            }
        }

        if (els) {
            // if it will always we 0, only the else block will ever be executed
            if (vx_Irblock_alwaysIsVar(cond, condVar, VX_IR_VALUE_IMM_INT(0))) {
                for (size_t i = 0; i < op->outs_len; i ++) {
                    const vx_IrVar out = op->outs[i].var;
                    vx_IrBlock_renameVar(block, out, els->outs[i], VX_RENAME_VAR_BOTH); // does all the bookkeeping for us
                }

                vx_IrBlock_insertAllOpAfter(block, pelse->block, op);
                vx_IrOp_removeParam(op, VX_IR_NAME_COND_THEN); // don't want to delete that block
                vx_IrOp_remove(op);
                continue;
            }
        }

        for (size_t i = 0; i < op->outs_len; i ++) {
            if (!vx_IrBlock_varUsed(root, op->outs[i].var)) {
                if (pthen)
                    vx_IrBlock_removeOutAt(pthen->block, i);
                if (pelse)
                    vx_IrBlock_removeOutAt(pelse->block, i);
                vx_IrOp_removeOutAt(op, i);
                i --;
            }
        }
    }
}
