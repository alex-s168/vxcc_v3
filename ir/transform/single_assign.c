#include "../cir.h"

// returns last var that was originally onVar 
vx_IrVar vx_CIrBlock_partial_mksaFinal_norec(vx_IrBlock* block, vx_IrVar onVar)
{
    vx_IrVar lastWithSame = onVar;
    for (vx_IrOp* op = block->first; op; op = op->next)
    {
        for (size_t i = 0; i < op->outs_len; i ++)
        {
            vx_IrVar* v = &op->outs[i].var;
            if (*v == onVar)
            {
                vx_IrVar new = vx_IrBlock_new_var(block, op);

                vx_IrOp *oldstart = block->first;
                    block->first = op->next;
                    vx_IrBlock_rename_var(block, *v, new);
                block->first = oldstart;

                lastWithSame = new;
            }
        }
    }
    return lastWithSame;
}

/**
 * Go trough every unconditional assignement in the block,
 * change the id of the vars to new,
 * unused vars and refactor every use AFTER the declaration;
 *
 * inside out!
 */
void vx_CIrBlock_mksa_final(vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        for (size_t j = 0; j < op->params_len; j ++)
            if (op->params[j].val.type == VX_IR_VAL_BLOCK)
                vx_CIrBlock_mksa_final(op->params[j].val.block);

        for (size_t j = 0; j < op->outs_len; j ++) {
            vx_IrVar *var = &op->outs[j].var;
            const vx_IrVar old = *var;
            const vx_IrVar new = vx_IrBlock_new_var(block, op);
            *var = new;

            vx_IrOp *oldstart = block->first;
                block->first = op->next;
                vx_IrBlock_rename_var(block, old, new);
            block->first = oldstart;
        }
    }
}
