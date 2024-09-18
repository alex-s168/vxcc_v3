#include "../opt.h"

static void rmcode_before_label(vx_IrOp *op) {
    for (; op; op = op->next) {
        if (op->id == VX_IR_OP_LABEL)
            break;

        vx_IrOp_remove(op);
    }
}

void vx_opt_ll_dce(vx_IrBlock *block) {
    for (vx_IrOp *op = block->first; op; op = op->next)
        if (vx_IrOp_endsFlow(op))
            rmcode_before_label(op->next);
}

