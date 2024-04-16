#include "../opt.h"

void opt_loop_simplify(SsaView view, SsaBlock *block) {
    while (irview_find(&view, SSA_OP_FOR)) {
        SsaOp *op = (SsaOp *) irview_take(view);

        SsaBlock *cond = irop_param(op, SSA_NAME_COND)->block;
        opt(cond); // !
        const SsaVar condVar = cond->outs[0];

        // if it will always we 0, we optimize it out
        if (irblock_alwaysis_var(cond, condVar, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 })) {
            irview_replace(block, view, NULL, 0);

            goto next;
        }

        opt(irop_param(op, SSA_NAME_LOOP_DO)->block);

        // if it will never be 0 (not might be 0), it is always true => infinite loop
        if (!irblock_mightbe_var(cond, condVar, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 })) {
            SsaOp new;
            irop_init(&new, SSA_OP_INFINITE, block);
            irop_steal_param(&new, op, SSA_NAME_LOOP_START);
            irop_steal_param(&new, op, SSA_NAME_LOOP_DO);
            irop_steal_param(&new, op, SSA_NAME_LOOP_STRIDE);
            irop_steal_states(&new, op);
            irop_steal_outs(&new, op);

            irview_replace(block, view, &new, 1);

            goto next;
        }

        // if it is a less than, we can do a repeat
        if (cond->ops_len > 0 && cond->ops[0].id == SSA_OP_LT) {

            bool break2 = false;
            do {
                const SsaValue *a = irop_param(&cond->ops[0], SSA_NAME_OPERAND_A);
                // require it to be the counter
                if (a->type != SSA_VAL_VAR)
                    break;
                if (a->var != cond->ins[0])
                    break;
                if (cond->ops[0].outs[0].var != cond->outs[0])
                    break;
            
                const SsaValue b = irvalue_clone(*irop_param(&cond->ops[0], SSA_NAME_OPERAND_B));

                SsaOp new;
                irop_init(&new, SSA_OP_REPEAT, block);
                irop_steal_param(&new, op, SSA_NAME_LOOP_DO);
                irop_steal_param(&new, op, SSA_NAME_LOOP_START);
                irop_add_param_s(&new, SSA_NAME_LOOP_ENDEX, b);
                irop_steal_param(&new, op, SSA_NAME_LOOP_STRIDE);
                irop_steal_states(&new, op); // steal all state init params
                irop_steal_outs(&new, op);
                // we probably do a bit of memory leaking here...
                *op = new;
                
                break2 = true;
            } while(0);
            if (break2)
                goto next; // we optimized already
        }

    next:
        view = irview_drop(view, 1);
    }
}
