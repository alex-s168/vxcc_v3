#include "../opt.h"

void vx_opt_vars(vx_IrBlock *block) {
    vx_IrBlock *root = vx_IrBlock_root(block);
    assert(root != NULL);

    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (vx_IrOp_isVolatile(op) || vx_IrOp_hasSideEffect(op))
            continue;

        bool can_rem = true;
        for (size_t i = 0; i < op->outs_len; i ++) {
            vx_IrVar out = op->outs[i].var;

            if (vx_IrBlock_varUsed(root, out)) {
                can_rem = false;
                break;
            }
        }

        if (can_rem) {
            vx_IrOp_remove(op);
        }
    }
}
