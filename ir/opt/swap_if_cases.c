#include "../passes.h"

// if the else case only has one op, swap both cases

void vx_opt_if_swapCases(vx_CU* cu, vx_IrBlock* block)
{
    for (vx_IrOp* op = block->first; op; op = op->next)
    {
        if (op->id != VX_IR_OP_IF)
            continue;

        vx_IrBlock** belse = &vx_IrOp_param(op, VX_IR_NAME_COND_ELSE)->block;

        if (!(*belse && (*belse)->first && !(*belse)->first->next))
            continue;

        vx_IrOpType elseOpTy = (*belse)->first->id;

        switch (elseOpTy)
        {
            case VX_IR_OP_BREAK:
            case VX_IR_OP_CONTINUE:
            case VX_IR_OP_CONDTAILCALL:
                break;
            default:
                continue;
        }

        vx_IrBlock** bthen = &vx_IrOp_param(op, VX_IR_NAME_COND_THEN)->block;

        // swap 
        {
            vx_IrBlock* temp = *bthen;
            *bthen = *belse;
            *belse = temp;
        }

        // invert condition
    
        vx_IrVar* cond = &vx_IrOp_param(op, VX_IR_NAME_COND)->var; 

        vx_IrOp* pred = vx_IrOp_predecessor(op);

        // generate not op; optimized away by ther pass
        vx_IrOp* inv = vx_IrBlock_insertOpCreateAfter(block, pred, VX_IR_OP_NOT);
        vx_IrVar new = vx_IrBlock_newVar(block, inv);
        vx_IrOp_addOut(inv, new, vx_IrBlock_typeofVar(block, *cond));
        vx_IrOp_addParam_s(inv, VX_IR_NAME_VALUE, VX_IR_VALUE_VAR(*cond));

        *cond = new;
    }
}
