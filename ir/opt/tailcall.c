#include <assert.h>

#include "../opt.h"

static bool trav(vx_IrOp *op, void *ignore) {
    (void) ignore;
    if (op->id == VX_IR_OP_CALL && vx_IrOp_is_tail(op)) {
        op->id = VX_IR_OP_TAILCALL;
    }
    return false;
}

void vx_opt_ll_tailcall(vx_IrBlock *block) {
    vx_IrBlock_deep_traverse(block, trav, NULL); 
}

void vx_opt_ll_condtailcall(vx_IrBlock *block) {
    assert(block->is_root);

    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->id != VX_LIR_OP_COND)
            continue;

        size_t label = vx_IrOp_param(op, VX_IR_NAME_ID)->id;
        vx_IrOp *label_op = block->as_root.labels[label].decl;
        vx_IrOp *tailcall = label_op ? label_op->next : NULL;
        if (tailcall == NULL)
            continue;

        if (tailcall->id != VX_IR_OP_TAILCALL)
            continue;

        vx_IrValue cond = *vx_IrOp_param(op, VX_IR_NAME_COND);
        vx_IrOp_remove_param(op, VX_IR_NAME_COND); // don't want it to free that
        vx_IrOp *nxt = op->next;
        vx_IrOp_destroy(op);
        vx_IrOp_init(op, VX_IR_OP_CONDTAILCALL, block);
        op->next = nxt;
        vx_IrOp_add_param_s(op, VX_IR_NAME_COND, cond);

        vx_IrOp_steal_param(op, tailcall, VX_IR_NAME_ADDR);

        vx_IrOp_remove(tailcall);
    }
}
