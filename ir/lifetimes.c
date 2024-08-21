#include <assert.h>
#include "ir.h"
#include "llir.h"

// TODO: multipart lifetimes because jumps

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

        vx_IrOp *start = decl;
        vx_IrOp *end = decl;
        for (vx_IrOp *op = block->first; op; op = op->next) {
            if (vx_IrOp_var_used(op, var)) {
                if (vx_IrOp_after(op, end))
                    end = op;
                else if (vx_IrOp_after(start, op))
                    start = op;
            }
        }

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
