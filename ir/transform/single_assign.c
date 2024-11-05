#include "../passes.h"

/**
 * Go trough every unconditional assignement in the block,
 * change the id of the vars to new,
 * unused vars and refactor every use AFTER the declaration;
 *
 * inside out!
 */
void vx_CIrBlock_mksa_final(vx_CU* cu, vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next)
    {
        FOR_INPUTS(op, inp, ({
            if (inp.type == VX_IR_VAL_BLOCK)
                vx_CIrBlock_mksa_final(cu, inp.block);
        }));

        for (size_t j = 0; j < op->outs_len; j ++) {
            vx_IrVar *var = &op->outs[j].var;
            const vx_IrVar old = *var;
            const vx_IrVar new = vx_IrBlock_newVar(block, op);
            *var = new;

            vx_IrOp *oldstart = block->first;
                block->first = op->next;
                vx_IrBlock_renameVar(block, old, new, VX_RENAME_VAR_OUTPUTS);
            block->first = oldstart;
        }
    }
}
