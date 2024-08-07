#include "../llir.h"

void vx_IrBlock_ll_cmov_expand(vx_IrBlock *block) {
    for (vx_IrOp* op = block->first; op; op = op->next) {
        if (op->id != VX_IR_OP_CMOV)
            continue;

        vx_IrTypedVar out = op->outs[0];
        vx_IrValue vthen = *vx_IrOp_param(op, VX_IR_NAME_COND_THEN);
        vx_IrValue velse = *vx_IrOp_param(op, VX_IR_NAME_COND_ELSE);

        vx_IrOp* pred = vx_IrOp_predecessor(op);

        vx_IrOp* new = fastalloc(sizeof(vx_IrOp));

        vx_IrOp_init(new, VX_IR_OP_IMM, block);
        vx_IrOp_add_out(new, out.var, out.type);
        vx_IrOp_add_param_s(new, VX_IR_NAME_VALUE, velse);

        if (pred) {
            new->next = op;
            pred->next = new;
        } else {
            new->next = block->first;
            block->first = new;
        }

        vx_IrOp_remove_param(op, VX_IR_NAME_COND_ELSE);
    }
}
