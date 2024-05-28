#include "../opt.h"

void vx_opt_loop_simplify(vx_IrView view,
                          vx_IrBlock *block)
{
    while (vx_IrView_find(&view, VX_IR_OP_FOR)) {
        vx_IrOp *op = (vx_IrOp *) vx_IrView_take(view);

        vx_IrBlock *cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;
        const vx_IrVar condVar = cond->outs[0];

        // if it will always we 0, we optimize it out
        if (vx_Irblock_alwaysis_var(cond, condVar, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 0 })) {
            vx_IrView_replace(block, view, NULL, 0);

            goto next;
        }

        // if it will never be 0 (not might be 0), it is always true => infinite loop
        if (!vx_Irblock_mightbe_var(cond, condVar, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 0 })) {
            vx_IrOp new;
            vx_IrOp_init(&new, VX_IR_OP_INFINITE, block);
            vx_IrOp_steal_param(&new, op, VX_IR_NAME_LOOP_START);
            vx_IrOp_steal_param(&new, op, VX_IR_NAME_LOOP_DO);
            vx_IrOp_steal_param(&new, op, VX_IR_NAME_LOOP_STRIDE);
            vx_IrOp_steal_states(&new, op);
            vx_IrOp_steal_outs(&new, op);

            vx_IrView_replace(block, view, &new, 1);

            goto next;
        }

        // if it is a less than, we can do a repeat
        if (cond->ops_len > 0 && cond->ops[0].id == VX_IR_OP_LT) {

            bool break2 = false;
            do {
                const vx_IrValue *a = vx_IrOp_param(&cond->ops[0], VX_IR_NAME_OPERAND_A);
                // require it to be the counter
                if (a->type != VX_IR_VAL_VAR)
                    break;
                if (a->var != cond->ins[0])
                    break;
                if (cond->ops[0].outs[0].var != cond->outs[0])
                    break;
            
                const vx_IrValue b = *vx_IrOp_param(&cond->ops[0], VX_IR_NAME_OPERAND_B);

                vx_IrOp new;
                vx_IrOp_init(&new, VX_IR_OP_REPEAT, block);
                vx_IrOp_steal_param(&new, op, VX_IR_NAME_LOOP_DO);
                vx_IrOp_steal_param(&new, op, VX_IR_NAME_LOOP_START);
                vx_IrOp_add_param_s(&new, VX_IR_NAME_LOOP_ENDEX, b);
                vx_IrOp_steal_param(&new, op, VX_IR_NAME_LOOP_STRIDE);
                vx_IrOp_steal_states(&new, op); // steal all state init params
                vx_IrOp_steal_outs(&new, op);
                // we probably do a bit of memory leaking here...
                *op = new;
                
                break2 = true;
            } while(0);
            if (break2)
                goto next; // we optimized already
        }

    next:
        view = vx_IrView_drop(view, 1);
    }
}
