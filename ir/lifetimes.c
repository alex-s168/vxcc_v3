#include <assert.h>
#include "ir.h"
#include "llir.h"

/** only for root blocks */
void vx_IrBlock_lifetimes(vx_IrBlock *block) {
    assert(block->is_root);

    for (vx_IrVar var = 0; var < block->as_root.vars_len; var ++) {
        // llir shouldn't be nested 

        vx_IrOp *start = (void*) -1;
        vx_IrOp *end = 0;
        for (size_t i = 0; i < block->ops_len; i ++) {
            vx_IrOp *op = &block->ops[i];
            if (vx_IrOp_var_used(op, var)) {
                if (op > end)
                    end = op;
                else if (op < start)
                    start = op;
            }
        }

        lifetime lt;
        lt.first = start;
        lt.last = end;

        block->as_root.vars[var].ll_lifetime = lt;
    }
}
