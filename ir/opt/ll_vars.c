#include "../opt.h"

// TODO: remove ssa var rem pass and replace with this

void vx_opt_ll_vars(vx_IrView view, vx_IrBlock *block) {
    const vx_IrBlock *root = vx_IrBlock_root(block);
    if (root == NULL)
        root = block;
    for (size_t i = 0; i < block->ops_len; i ++) {
        vx_IrOp *op = &view.block->ops[i];

        if (vx_IrOp_is_volatile(op))
            continue;

        size_t old_outs_len = op->outs_len;
        op->outs_len = 0;
        bool can_rem = true;
        for (size_t i = 0; i < old_outs_len; i ++) {
            vx_IrVar out = op->outs[i].var;

            if (vx_IrBlock_var_used(root, out)) {
                can_rem = false;
                break;
            }
        }
        op->outs_len = old_outs_len;
        if (!can_rem)
            continue;

        vx_IrOp_destroy(op);
        vx_IrOp_init(op, VX_IR_OP_NOP, block);
    }
}
