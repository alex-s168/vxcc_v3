#include <assert.h>

#include "../opt.h"

void vx_opt_remove_vars(vx_IrBlock *block)
{
    assert(block->is_root);

    for (size_t i = 0; i < block->as_root.vars_len; i ++) {
        vx_IrOp *decl = block->as_root.vars[i].decl;
        if (decl == NULL)
            continue;

        if (decl->id == VX_IR_OP_IF)
            continue; // we have a specific vx_opt_reduce_if

        // TODO: optimize away states and similar

        // we can't optimize away loop counters and similar
        bool in_outs = false;
        for (size_t j = 0; j < decl->outs_len; j ++) {
            if (decl->outs[j].var == i) {
                in_outs = true;
                break;
            }
        }
        if (!in_outs)
            continue;

        // we can't optimize away if we need other results from the operation
        bool can_rem = true;
        for (size_t j = 0; j < decl->outs_len; j ++) {
            if (vx_IrBlock_var_used(block, decl->outs[j].var)) {
                can_rem = false;
                break;
            }
        }

        if (!can_rem)
            continue;

        // in-place remove
        vx_IrOp_destroy(decl);
        vx_IrOp_init(decl, VX_IR_OP_NOP, decl->parent);

        for (size_t j = 0; j < decl->outs_len; j ++)
            block->as_root.vars[decl->outs[j].var].decl = NULL;
    }
}
