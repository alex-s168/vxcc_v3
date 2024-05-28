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

