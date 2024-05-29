#include "../opt.h"

void vx_opt_loop_simplify(vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->id != VX_IR_OP_FOR)
            continue;

        vx_IrBlock *cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;
        const vx_IrVar condVar = cond->outs[0];

        // if it will always we 0, we optimize it out
        if (vx_Irblock_alwaysis_var(cond, condVar, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 0 })) {
            vx_IrOp_remove(op);
            continue;
        }

        // if it will never be 0 (not might be 0), it is always true => infinite loop
        if (!vx_Irblock_mightbe_var(cond, condVar, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 0 })) {
            op->id = VX_IR_OP_INFINITE; // we could remove cond param
            continue;
        }

        // if it is a less than, we can do a repeat
        if (cond->first && cond->first->id == VX_IR_OP_LT) {
            const vx_IrValue *a = vx_IrOp_param(cond->first, VX_IR_NAME_OPERAND_A);
            // require it to be the counter
            if (a->type != VX_IR_VAL_VAR)
                continue;
            if (a->var != cond->ins[0].var)
                continue;
            if (cond->first->outs[0].var != cond->outs[0])
                continue;

            const vx_IrValue b = *vx_IrOp_param(cond->first, VX_IR_NAME_OPERAND_B);

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
        }
    }
}
