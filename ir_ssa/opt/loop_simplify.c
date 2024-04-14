#include "../opt.h"

void opt_loop_simplify(SsaView view, SsaBlock *block) {
    while (ssaview_find(&view, SSA_OP_FOR)) {
        const SsaOp *op = ssaview_take(view);

        SsaBlock *cond = ssaop_param(op, "cond")->block;
        opt(cond); // !
        const SsaVar condVar = cond->outs[0];

        // if it will never be 0 (not might be 0), it is always true => infinite loop
        if (!ssablock_mightbe_var(cond, condVar, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 })) {
            SsaOp new;
            ssaop_init(&new, SSA_OP_INFINITE);
            ssaop_steal_param(&new, op, "init");
            ssaop_steal_param(&new, op, "do");
            ssaop_steal_param(&new, op, "stride");
            ssaop_steal_all_params_starting_with(&new, op, "state"); // steal all state init params
            ssaop_steal_outs(&new, op);

            ssaview_replace(block, view, &new, 1);
            
            goto next;
        }

        // if it will always we 0, we optimize it out
        if (ssablock_alwaysis_var(cond, condVar, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 })) {
            ssaview_replace(block, view, NULL, 0);

            goto next;
        }

        // if it is a less than, we can do a repeat
        if (cond->ops_len > 0 && cond->ops[0].id == SSA_OP_LT) {
            bool break2 = false;
            do {
                const SsaValue *a = ssaop_param(&cond->ops[0], "a");
                // require it to be the counter
                if (a->type != SSA_VAL_VAR)
                    break;
                if (a->var != cond->ins[0])
                    break;
                if (cond->ops[0].outs[0].var != cond->outs[0])
                    break;
            
                const SsaValue *b = ssaop_param(&cond->ops[0], "b");

                SsaOp new;
                ssaop_init(&new, SSA_OP_REPEAT);
                ssaop_steal_param(&new, op, "do");
                ssaop_add_param_s(&new, "start", *ssaop_param(op, "init"));
                ssaop_add_param_s(&new, "endEx", *b);
                ssaop_steal_param(&new, op, "stride");
                ssaop_steal_all_params_starting_with(&new, op, "state"); // steal all state init params
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
