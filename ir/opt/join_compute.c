#include "../opt.h"
#include <assert.h>
#include <string.h>

static bool eq(vx_IrOp* a, vx_IrOp* b)
{
    if (!( a->id == b->id
        && a->args_len == b->args_len
        && a->params_len == b->params_len
        && a->outs_len == b->outs_len
        ))
        return false;

    for (size_t i = 0; i < a->params_len; i ++) {
        if (a->params[i].name != b->params[i].name)
            return false;
        if (!vx_IrValue_eq(a->params[i].val, b->params[i].val))
            return false;
    }

    for (size_t i = 0; i < a->args_len; i ++)
        if (!vx_IrValue_eq(a->args[i], b->args[i]))
            return false;

    return true;
}

static vx_IrOp* findMatchingOutBefore(vx_IrBlock* block, vx_IrOp* before, vx_IrOp* cmp, bool stepIn, bool stepOut)
{
    for (vx_IrOp* op = block->first; op && op != before; op = op->next)
    {
        if (eq(op, cmp))
            return op;

        if (stepIn) {
            FOR_INPUTS(op, inp, ({
                if (inp.type == VX_IR_VAL_BLOCK) {
                    vx_IrOp* m = findMatchingOutBefore(inp.block, NULL, cmp, /*stepIn=*/true, /*stepOut=*/false);
                    if (m) 
                        return m;
                }
            }));
        }
    }

    if (stepOut && block->parent)
    {
        vx_IrOp* m = findMatchingOutBefore(block->parent, block->parent_op, cmp, /*stepIn=*/false, /*stepOut=*/true);
        if (m)
            return m;
    }

    return NULL;
}

/**
 * Find identical computations and join them
 */
void vx_opt_join_compute(vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (vx_IrOp_isVolatile(op) || vx_IrOp_hasSideEffect(op))
            continue;

        vx_IrOp* m = findMatchingOutBefore(block, op, op, true, true);
        assert(m != op);
        if (m == NULL)
            continue;

        vx_IrOp* pred = vx_IrOp_predecessor(op);

        vx_IrOp* last = pred;
        for (size_t i = 0; i < op->outs_len; i ++) {
            vx_IrOp* new = vx_IrBlock_insertOpCreateAfter(block, pred, VX_IR_OP_IMM);

            vx_IrOp_addParam_s(new, VX_IR_NAME_VALUE, VX_IR_VALUE_VAR(m->outs[i].var));
            vx_IrOp_addOut(new, op->outs[i].var, op->outs[i].type);
            last = new;
        }

        last->next = op->next;
    }
}
