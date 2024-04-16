#include "../opt.h"

void opt_loop_simplify(SsaView view, SsaBlock *block) {
    while (ssaview_find(&view, SSA_OP_FOR)) {
        const SsaOp *op = ssaview_take(view);

        SsaBlock *cond = ssaop_param(op, SSA_NAME_COND)->block;
        opt(cond); // !
        const SsaVar condVar = cond->outs[0];

        // if it will always we 0, we optimize it out
        if (ssablock_alwaysis_var(cond, condVar, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 })) {
            ssaview_replace(block, view, NULL, 0);

            goto next;
        }

        opt(ssaop_param(op, SSA_NAME_LOOP_DO)->block);

        // if it will never be 0 (not might be 0), it is always true => infinite loop
        if (!ssablock_mightbe_var(cond, condVar, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 })) {
            SsaOp new;
            ssaop_init(&new, SSA_OP_INFINITE, block);
            ssaop_steal_param(&new, op, SSA_NAME_LOOP_START);
            ssaop_steal_param(&new, op, SSA_NAME_LOOP_DO);
            ssaop_steal_param(&new, op, SSA_NAME_LOOP_STRIDE);
            ssaop_steal_states(&new, op);
            ssaop_steal_outs(&new, op);

            ssaview_replace(block, view, &new, 1);

            goto next;
        }

        // if it is a less than, we can do a repeat
        if (cond->ops_len > 0 && cond->ops[0].id == SSA_OP_LT) {

            bool break2 = false;
            do {
                const SsaValue *a = ssaop_param(&cond->ops[0], SSA_NAME_OPERAND_A);
                // require it to be the counter
                if (a->type != SSA_VAL_VAR)
                    break;
                if (a->var != cond->ins[0])
                    break;
                if (cond->ops[0].outs[0].var != cond->outs[0])
                    break;
            
                const SsaValue b = ssavalue_clone(*ssaop_param(&cond->ops[0], SSA_NAME_OPERAND_B));

                SsaOp new;
                ssaop_init(&new, SSA_OP_REPEAT, block);
                ssaop_steal_param(&new, op, SSA_NAME_LOOP_DO);
                ssaop_steal_param(&new, op, SSA_NAME_LOOP_START);
                ssaop_add_param_s(&new, SSA_NAME_LOOP_ENDEX, b);
                ssaop_steal_param(&new, op, SSA_NAME_LOOP_STRIDE);
                ssaop_steal_states(&new, op); // steal all state init params
                ssaop_steal_outs(&new, op);

                ssaview_replace(block, view, &new, 1);
                
                break2 = true;
            } while(0);
            if (break2)
                goto next; // we optimized already
        }

    next:
        view = ssaview_drop(view, 1);
    }
}
