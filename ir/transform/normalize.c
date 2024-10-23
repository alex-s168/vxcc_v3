#include <assert.h>
#include <stdlib.h>

#include "../cir.h"

void vx_CIrBlock_normalize(vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->id == VX_IR_OP_CFOR) {
            vx_IrBlock *new = vx_IrBlock_initHeap(block, op);

            vx_IrBlock *b_init = vx_IrOp_param(op, VX_IR_NAME_LOOP_START)->block;
            vx_IrBlock *b_cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;
            vx_IrBlock *b_end = vx_IrOp_param(op, VX_IR_NAME_LOOP_ENDEX)->block;
            vx_IrBlock *b_do = vx_IrOp_param(op, VX_IR_NAME_LOOP_DO)->block;
            
            free(op->params);
            op->params = NULL;
            op->params_len = 0;

            vx_IrBlock_addAllOp(new, b_init);

            vx_IrOp *opwhile = vx_IrBlock_addOpBuilding(new);
            vx_IrOp_init(opwhile, VX_IR_OP_WHILE, new);
            vx_IrOp_addParam_s(opwhile, VX_IR_NAME_COND, VX_IR_VALUE_BLK(b_cond));

            vx_IrBlock *doblock = vx_IrBlock_initHeap(new, opwhile);
            vx_IrBlock_addAllOp(doblock, b_do);
            vx_IrBlock_addAllOp(doblock, b_end);

            vx_IrOp_addParam_s(opwhile, VX_IR_NAME_LOOP_DO, VX_IR_VALUE_BLK(doblock));

            vx_IrBlock_insertAllOpAfter(block, new, op);
            vx_IrOp_remove(op);
        }

        FOR_INPUTS(op, inp, {
            if (inp.type == VX_IR_VAL_BLOCK)
                vx_CIrBlock_normalize(inp.block);
        });
    }
}
