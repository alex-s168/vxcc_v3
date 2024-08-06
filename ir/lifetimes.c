#include <assert.h>
#include "ir.h"
#include "llir.h"

/** only for root blocks */
void vx_IrBlock_lifetimes(vx_IrBlock *block) {
    assert(block->is_root);

    for (vx_IrVar var = 0; var < block->as_root.vars_len; var ++) {
        vx_IrOp *decl = block->as_root.vars[var].decl;
        if (decl == NULL) {
            block->as_root.vars[var].ll_lifetime.first = NULL;
            continue;
        }

        // llir shouldn't be nested 

        vx_IrOp *start = (void*) -1;
        vx_IrOp *end = 0;
        for (vx_IrOp *op = block->first; op; op = op->next) {
            if (vx_IrOp_var_used(op, var)) {
                if (op > end)
                    end = op;
                else if (op < start)
                    start = op;
            }
        }

        if (start == (void*)-1)
            start = decl;

        if (end == 0)
            end = decl;

        lifetime lt;
        lt.first = start;
        lt.last = end;

        block->as_root.vars[var].ll_lifetime = lt;
    }

    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->id == VX_IR_OP_PLACE) {
            vx_IrVar var = vx_IrOp_param(op, VX_IR_NAME_VAR)->var;
            block->as_root.vars[var].ever_placed = true;
        }
    }
}
