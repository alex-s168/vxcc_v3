#include "../opt.h"

static void trav(vx_IrOp *op,
                 void *data)
{
    vx_IrBlock *block = data;

    if (op->id != VX_IR_OP_IMM)
        return;

    const vx_IrValue value = *vx_IrOp_param(op, VX_IR_NAME_VALUE);

    for (size_t i = 0; i < op->outs_len; i ++) {
        const vx_IrVar out = op->outs[i].var;
        vx_IrView_substitute_var(vx_IrView_of_all(block), block, out, value);
    }
}

void vx_opt_inline_vars(vx_IrView view,
                        vx_IrBlock *block)
{
    vx_IrView_deep_traverse(view, trav, block);
}
