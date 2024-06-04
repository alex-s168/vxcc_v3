#include "../opt.h"

void vx_opt_vars(vx_IrBlock *block) {
    vx_IrBlock *root = vx_IrBlock_root(block);
    if (root == NULL)
        root = block;

    for (vx_IrOp *op = block->first; op; op = op->next) {
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

        vx_IrOp_remove(op);
    }
}
