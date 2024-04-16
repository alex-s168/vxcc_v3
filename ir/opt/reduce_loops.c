#include <assert.h>

#include "../opt.h"

/**
* Detect:
* \code
* %0 = while state0=0 cond=(%1){%1 < 10} (%2){
*   %3 = add a=%2 b=1;
*   ^ %3
* };
* \endcode
*
* To:
* \code
* for start=0 cond=(%0){%0 < 10} stride=1 (%2){
*   ...
* }
* \endcode
*
* We only care about the first increment of i in the loop.
*
* Problems:
* - We need to detect which state is the iteration counter
* - We need to extract that
*
* The iteration counter has a unconditional increment by STRIDE in the loop
* It is also checked in the condition.
*
* But when the condition is always true, we need to make it an infinite loop!
*/
void opt_reduce_loops(SsaView view, SsaBlock *block) {
    assert(view.block == block);
    
    while (irview_find(&view, SSA_OP_FOR)) {
        SsaOp *op = (SsaOp *) irview_take(view);

        if (op->id == SSA_OP_WHILE && op->outs_len > 0) { // "fast path" for when we don't even have states
            SsaBlock **bdo = &irop_param(op, SSA_NAME_LOOP_DO)->block;
            SsaBlock *newdo = *bdo;
            SsaBlock **bcond = &irop_param(op, SSA_NAME_COND)->block;
            SsaBlock *newcond = *bcond;

            // TODO: if we detect multiple counters here, chose the one which is single present in the condition
            
            SsaOp *incOp = NULL;
            size_t stateId;
            struct SsaStaticIncrement si;
            for (stateId = 0; stateId < op->outs_len; stateId ++) {
                bool found = false;
                for (size_t j = 0; j < newdo->ops_len; j ++) {
                    incOp = &newdo->ops[j];
                    si = irop_detect_static_increment(incOp);
                    if (si.detected &&
                        incOp->outs[0].var == op->outs[stateId].var &&
                        si.var == newcond->ins[stateId])
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
                incOp = NULL;
            }

            if (incOp == NULL)
                continue;
            
            *bcond = NULL;
            *bdo = NULL;

            const SsaValue init = op->states[stateId];

            // counter needs to be at pos 0 oviously
            irblock_swap_state_at(newcond, stateId, 0);
            irop_remove_state_at(op, stateId);

            SsaOp new;
            irop_init(&new, SSA_OP_FOR, block);
            irop_add_param_s(&new, SSA_NAME_LOOP_START, init);
            irop_add_param_s(&new, SSA_NAME_LOOP_STRIDE, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = si.by });
            irop_steal_states(&new, op);
            irop_add_param_s(&new, SSA_NAME_COND, (SsaValue) { .type = SSA_VAL_BLOCK, .block = newcond });
            irop_add_param_s(&new, SSA_NAME_LOOP_DO, (SsaValue) { .type = SSA_VAL_BLOCK, .block = newdo });

            irop_destroy(incOp);
            irop_init(incOp, SSA_OP_NOP, block);

            irop_destroy(op);
            (void) irview_replace(block, view, &new, 1);
        }
        
        view = irview_drop(view, 1);
    }
}
