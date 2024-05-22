#include "../ir.h"

static void lower_into(vx_IrBlock *block, vx_IrBlock *dest) {
    for (size_t i = 0; i < block->ops_len; i ++) {
        vx_IrOp * op = &block->ops[i];

        if (op->id == VX_IR_OP_IF) {
            vx_IrBlock *cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;
            
            vx_IrBlock *then = NULL; {
                vx_IrValue *val = vx_IrOp_param(op, VX_IR_NAME_COND_THEN);
                if (val)
                    then = val->block;
            }

            vx_IrBlock *els = NULL; {
                vx_IrValue *val = vx_IrOp_param(op, VX_IR_NAME_COND_ELSE);
                if (val)
                    then = val->block;
            }

            // TODO: remove not here if only then and move higher up in the pipeline: invert condition and use else branch instead

            if (els == NULL && then == NULL) {
                continue;
            }

            vx_IrVar cond_var = cond->outs[0];
            vx_IrBlock_add_all_op(dest, cond);

            if (els == NULL) {
                // we only have a then block 
                // create a label after the then block, invert condition, conditional jump there

                /* cond_var = !cond_var */ {
                    vx_IrOp *inv = dest->ops + dest->ops_len;
                    vx_IrBlock_add_op(dest, (vx_IrOp[1]) {{}});
                    vx_IrVar new = vx_IrBlock_new_var(dest, inv);
                    vx_IrOp_init(inv, VX_IR_OP_NOT, block);
                    vx_IrOp_add_out(inv, new, vx_IrBlock_typeof_var(cond, cond_var));
                    vx_IrOp_add_param_s(inv, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = cond_var });
                    cond_var = new;
                }

                // TODO 
            }
            else if (then == NULL) {
                // TODO 
            }
            else {
                // TODO
            }
        }
        else {
            vx_IrBlock_add_op(dest, op); 
        }
    }
}

void vx_IrBlock_llir_lower(vx_IrBlock *block) {

}
