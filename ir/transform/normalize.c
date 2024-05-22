#include <assert.h>
#include <stdlib.h>

#include "../cir.h"

void vx_CIrBlock_normalize(vx_IrBlock *block)
{
    assert(block->is_root);

    for (size_t i = 0; i < block->ins_len; i ++) {
        vx_IrOp *op = block->ops;

        if (op->id == VX_CIR_OP_CFOR) {
            vx_IrBlock *new = vx_IrBlock_init_heap(block, i);

            vx_IrBlock *b_init = vx_IrOp_param(op, VX_IR_NAME_LOOP_START)->block;
            vx_IrBlock *b_cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;
            vx_IrBlock *b_end = vx_IrOp_param(op, VX_IR_NAME_LOOP_ENDEX)->block;
            vx_IrBlock *b_do = vx_IrOp_param(op, VX_IR_NAME_LOOP_DO)->block;
            // we free manually because we don't want to free blocks if should_free
            // TODO: ^ is stupid
            free(op->outs);
            free(op->states);
            vx_IrOp_remove_params(op);

            {
                vx_IrOp opdo;
                vx_IrOp_init(&opdo, VX_IR_OP_FLATTEN_PLEASE, new);
                vx_IrOp_add_param_s(&opdo, VX_IR_NAME_BLOCK, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = b_init });
                vx_IrBlock_add_op(new, &opdo);
            }

            vx_IrOp opwhile;
            vx_IrOp_init(&opwhile, VX_IR_OP_WHILE, new);
            vx_IrOp_add_param_s(&opwhile, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = b_cond });

            vx_IrBlock *doblock = vx_IrBlock_init_heap(new, new->ops_len);
            {
                vx_IrOp opdo;
                vx_IrOp_init(&opdo, VX_IR_OP_FLATTEN_PLEASE, new);
                vx_IrOp_add_param_s(&opdo, VX_IR_NAME_BLOCK, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = b_do });
                vx_IrBlock_add_op(doblock, &opdo);
            }
            {
                vx_IrOp opdo;
                vx_IrOp_init(&opdo, VX_IR_OP_FLATTEN_PLEASE, new);
                vx_IrOp_add_param_s(&opdo, VX_IR_NAME_BLOCK, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = b_end });
                vx_IrBlock_add_op(doblock, &opdo);
            }

            vx_IrOp_add_param_s(&opwhile, VX_IR_NAME_LOOP_DO, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = doblock });

            vx_IrBlock_add_op(new, &opwhile);

            op->id = VX_IR_OP_FLATTEN_PLEASE;
            vx_IrOp_add_param_s(op, VX_IR_NAME_BLOCK, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = new });
        }
    }
}
