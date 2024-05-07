#include "../cir.h"

/**
 * Go trough every unconditional assignement in the block,
 * change the id of the vars to new,
 * unused vars and refactor every use AFTER the declaration;
 *
 * inside out!
 */
void vx_CIrBlock_mksa_final(block)
    vx_IrBlock *block;
{
    for (size_t i = 0; i < block->ops_len; i ++) {
        vx_IrOp *op = &block->ops[i];

        for (size_t j = 0; j < op->params_len; j ++)
            if (op->params[j].val.type == VX_IR_VALBLOCK)
                vx_CIrBlock_mksa_final(op->params[j].val.block);

        for (size_t j = 0; j < op->outs_len; j ++) {
            vx_IrVar *var = &op->outs[j].var;
            const vx_IrVar old = *var;
            const vx_IrVar new = vx_IrBlock_new_var(block, op);
            *var = new;
            vx_IrView view = vx_IrView_of_all(block);
            view.start = i + 1;
            vx_IrView_rename_var(view, block, old, new);
        }
    }
}
