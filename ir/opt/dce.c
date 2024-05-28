#include "../opt.h"

static void rmcode_before_label(vx_IrBlock *block, size_t first) {
    for (size_t i = first; i < block->ops_len; i ++) {
        vx_IrOp *op = &block->ops[i];

        if (op->id == VX_LIR_OP_LABEL)
            break;

        vx_IrOp_destroy(op);
        vx_IrOp_init(op, VX_IR_OP_NOP, block);
    }
}

void vx_opt_ll_dce(vx_IrBlock *block) {
    for (size_t i = 0; i < block->ops_len; i ++) {
        vx_IrOp *op = &block->ops[i];
        if (vx_IrOp_ends_flow(op)) {
            rmcode_before_label(block, i + 1);
        }
    }
}

