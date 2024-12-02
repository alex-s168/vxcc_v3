#include <assert.h>
#include "../passes.h"

static bool trav(vx_IrOp *op, void *ignore) 
{
    (void) ignore;
    if (op->id == VX_IR_OP_CALL && vx_IrOp_isTail(op)) {
        op->id = VX_IR_OP_TAILCALL;
    }
    return false;
}

void vx_opt_ll_tailcall(vx_CU* cu, vx_IrBlock *block) 
{
    vx_IrBlock_deepTraverse(block, trav, NULL); 
}

