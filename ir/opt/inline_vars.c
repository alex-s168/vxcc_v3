#include "../opt.h"

static bool trav (vx_IrOp *op, void *data)
{
    vx_IrBlock *block = data;

    if (op->id != VX_IR_OP_IMM)
        return false;

    const vx_IrValue value = *vx_IrOp_param(op, VX_IR_NAME_VALUE);

    for (size_t i = 0; i < op->outs_len; i ++) {
        const vx_IrVar out = op->outs[i].var;
        vx_IrBlock_substitute_var(block, out, value);
    }

    return false;
}

void vx_opt_inline_vars(vx_IrBlock *block) {
    vx_IrBlock_deep_traverse(block, trav, block);
}
