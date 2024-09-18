#include <assert.h>
#include <stdlib.h>

#include "../cir.h"

void vx_CIrBlock_normalize(vx_IrBlock *block)
{
    assert(block->is_root);

    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->id == VX_CIR_OP_CFOR) {
            vx_IrBlock *new = vx_IrBlock_initHeap(block, op);

            vx_IrBlock *b_init = vx_IrOp_param(op, VX_IR_NAME_LOOP_START)->block;
            vx_IrBlock *b_cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;
            vx_IrBlock *b_end = vx_IrOp_param(op, VX_IR_NAME_LOOP_ENDEX)->block;
            vx_IrBlock *b_do = vx_IrOp_param(op, VX_IR_NAME_LOOP_DO)->block;
            
            free(op->params);
            op->params = NULL;
            op->params_len = 0;

            {
                vx_IrOp* opdo = vx_IrBlock_addOpBuilding(new);
                vx_IrOp_init(opdo, VX_IR_OP_FLATTEN_PLEASE, new);
                vx_IrOp_addParam_s(opdo, VX_IR_NAME_BLOCK, VX_IR_VALUE_BLK(b_init));
                vx_IrBlock_addOp(new, opdo);
            }

            vx_IrOp *opwhile = vx_IrBlock_addOpBuilding(new);
            vx_IrOp_init(opwhile, VX_IR_OP_WHILE, new);
            vx_IrOp_addParam_s(opwhile, VX_IR_NAME_COND, VX_IR_VALUE_BLK(b_cond));

            vx_IrBlock *doblock = vx_IrBlock_initHeap(new, opwhile);
            {
                vx_IrOp* opdo = vx_IrBlock_addOpBuilding(new);
                vx_IrOp_init(opdo, VX_IR_OP_FLATTEN_PLEASE, new);
                vx_IrOp_addParam_s(opdo, VX_IR_NAME_BLOCK, VX_IR_VALUE_BLK(b_do));
                vx_IrBlock_addOp(doblock, opdo);
            }
            {
                vx_IrOp* opdo = vx_IrBlock_addOpBuilding(new);
                vx_IrOp_init(opdo, VX_IR_OP_FLATTEN_PLEASE, new);
                vx_IrOp_addParam_s(opdo, VX_IR_NAME_BLOCK, VX_IR_VALUE_BLK(b_end));
                vx_IrBlock_addOp(doblock, opdo);
            }

            vx_IrOp_addParam_s(opwhile, VX_IR_NAME_LOOP_DO, VX_IR_VALUE_BLK(doblock));

            op->id = VX_IR_OP_FLATTEN_PLEASE;
            vx_IrOp_addParam_s(op, VX_IR_NAME_BLOCK, VX_IR_VALUE_BLK(new));
        }
    }
}
