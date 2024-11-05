#include "../passes.h"

static void rmcode_before_label(vx_IrOp *op) {
    for (; op; op = op->next) {
        if (op->id == VX_IR_OP_LABEL)
            break;

        vx_IrOp_remove(op);
    }
}

static bool labelUsed(vx_IrBlock* block, size_t label)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->id == VX_IR_OP_GOTO || op->id == VX_IR_OP_COND) {
            size_t id = vx_IrOp_param(op, VX_IR_NAME_ID)->id;
            if (id == label) return true;
        }
    }
    return false;
}

// TODO: rename to vx_opt_dce
void vx_opt_ll_dce(vx_CU* cu, vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->id == VX_IR_OP_LABEL) {
            size_t id = vx_IrOp_param(op, VX_IR_NAME_ID)->id;
            if (!labelUsed(block, id)) {
                vx_IrOp* pred = vx_IrOp_predecessor(op);
                if (pred == NULL) {
                    block->first = op->next;
                } else {
                    pred->next = op->next;
                }
                continue;
            }
        }
    }

    for (vx_IrOp *op = block->first; op; op = op->next)
        if (vx_IrOp_endsFlow(op))
            rmcode_before_label(op->next);
}

