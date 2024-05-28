#include <assert.h>

#include "../opt.h"

static void trav(vx_IrOp *op, void *ignore) {
    (void) ignore;
    if (op->id == VX_IR_OP_CALL && vx_IrOp_is_tail(op)) {
        op->id = VX_IR_OP_TAILCALL;
    }
}

void vx_opt_tailcall(vx_IrBlock *block) {
   vx_IrView_deep_traverse(vx_IrView_of_all(block), trav, NULL); 
}

void vx_opt_ll_condtailcall(vx_IrBlock *block) {
    assert(block->is_root);
    for (size_t i = 0; i < block->ops_len; i ++) {
        vx_IrOp *op = &block->ops[i];

        if (op->id != VX_LIR_COND)
            continue;

        size_t label = vx_IrOp_param(op, VX_IR_NAME_ID)->id;
        vx_IrOp *label_op = vx_IrBlock_root_get_label_decl(block, label);

        vx_IrOp *tailcall = vx_IrOp_next(label_op);
        if (tailcall == NULL)
            continue;

        if (tailcall->id != VX_IR_OP_TAILCALL)
            continue;

        // should not be anything that destroy would free
        vx_IrValue cond = *vx_IrOp_param(op, VX_IR_NAME_COND);

        vx_IrOp_destroy(op);
        vx_IrOp_init(op, VX_IR_OP_CONDTAILCALL, block);
        vx_IrOp_add_param_s(op, VX_IR_NAME_COND, cond);

        vx_IrOp_steal_param(op, tailcall, VX_IR_NAME_ADDR);

        vx_IrOp_destroy(tailcall);
        vx_IrOp_init(tailcall, VX_IR_OP_NOP, block);
    }
}
