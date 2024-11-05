#include <assert.h>
#include "ir.h"

static void extendLifetimeSegment(bool * bools, vx_IrOp * op)
{
    size_t last = 0;

    op = op->next;
    for (size_t i = 1; op; (op = op->next, i ++))
    {
        if (bools[i]) {
            last = i;
            break;
        }

        if (vx_IrOp_endsFlow(op)) {
            if (last != 0)
                last = i;
            break;
        }
    }

    for (size_t i = 0; i < last; i ++)
    {
        bools[i] = true;
    }
}

/** only for root blocks */
void vx_IrBlock_lifetimes(vx_IrBlock *block) {
    assert(block->is_root);
    size_t numOps = vx_IrBlock_countOps(block);

    for (vx_IrVar var = 0; var < block->as_root.vars_len; var ++)
    {
        lifetime* lt = &block->as_root.vars[var].ll_lifetime;
        lt->used_in_op = fastalloc(sizeof(bool) * numOps);

        size_t i = 0;
        for (vx_IrOp* op = block->first; op; (op = op->next, i ++))
        {
            lt->used_in_op[i] = vx_IrOp_varUsed(op, var) || vx_IrOp_varInOuts(op, var);
        }

        i = 0; 
        for (vx_IrOp* op = block->first; op; (op = op->next, i ++))
        {
            if (lt->used_in_op[i])
            {
                extendLifetimeSegment(&lt->used_in_op[i], op);
            }
        }

        /*
        printf("%%%zu = ", var);
        for (size_t i = 0; i < numOps; i ++) {
            printf("%i, ", lt->used_in_op[i]);
        }
        puts("");
        */
    }

    // completely unrelated
    for (vx_IrOp *op = block->first; op; op = op->next)
    {
        if (op->id == VX_IR_OP_PLACE)
        {
            vx_IrVar var = vx_IrOp_param(op, VX_IR_NAME_VAR)->var;
            block->as_root.vars[var].ever_placed = true;
        }
    }
}
