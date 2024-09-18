#include "../opt.h"


struct vx_IrView_substitute_var__data {
    vx_IrBlock *block;
    vx_IrVar old;
    vx_IrValue new;
};

static bool vx_IrView_substitute_var__trav(vx_IrOp *op, void *dataIn) {
    struct vx_IrView_substitute_var__data *data = dataIn;

    vx_IrBlock* block = op->parent;
    for (size_t j = 0; j < block->outs_len; j ++) {
        if (block->outs[j] == data->old) {
            if (data->new.type == VX_IR_VAL_VAR) {
                block->outs[j] = data->new.var;
            }
        }
    }

    for (size_t i = 0; i < op->params_len; i ++)
        if (op->params[i].val.type == VX_IR_VAL_VAR && op->params[i].val.var == data->old)
            op->params[i].val = data->new;

    return false;
}

static void vx_IrBlock_substitute_var(vx_IrBlock *block, vx_IrVar old, vx_IrValue new) {
    struct vx_IrView_substitute_var__data data = { .block = block, .old = old, .new = new };
    vx_IrBlock_deepTraverse(block, vx_IrView_substitute_var__trav, &data);
}

// =======================================================================

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
    vx_IrBlock_deepTraverse(block, trav, block);
}
