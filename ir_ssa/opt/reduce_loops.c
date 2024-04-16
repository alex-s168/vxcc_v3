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
    
    while (ssaview_find(&view, SSA_OP_FOR)) {
        SsaOp *op = (SsaOp *) ssaview_take(view);

        if (op->id == SSA_OP_WHILE && op->outs_len > 0) { // "fast path" for when we don't even have states
            SsaBlock **bdo = &ssaop_param(op, "do")->block;
            SsaBlock *newdo = *bdo;
            SsaBlock **bcond = &ssaop_param(op, "cond")->block;
            SsaBlock *newcond = *bcond;

            // TODO: if we detect multiple counters here, chose the one which is single present in the condition
            
            SsaOp *incOp = NULL;
            size_t stateId;
            struct SsaStaticIncrement si;
            for (stateId = 0; stateId < op->outs_len; stateId ++) {
                bool found = false;
                for (size_t j = 0; j < newdo->ops_len; j ++) {
                    incOp = &newdo->ops[j];
                    si = ssaop_detect_static_increment(incOp);
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

            static char buf[256];
            sprintf(buf, "state%zu", stateId);
            const SsaValue init = *ssaop_param(op, buf);

            // counter needs to be at pos 0 oviously
            ssablock_swap_state(newcond, stateId, 0);
            ssaop_drop_state_param(op, stateId);

            SsaOp new;
            ssaop_init(&new, SSA_OP_FOR, block);
            ssaop_add_param_s(&new, "init", init);
            ssaop_add_param_s(&new, "stride", (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = si.by });
            ssaop_steal_state_params(&new, op);
            ssaop_add_param_s(&new, "cond", (SsaValue) { .type = SSA_VAL_BLOCK, .block = newcond });
            ssaop_add_param_s(&new, "do", (SsaValue) { .type = SSA_VAL_BLOCK, .block = newdo });

            ssaop_destroy(incOp);
            ssaop_init(incOp, SSA_OP_NOP, block);

            ssaop_destroy(op);
            (void) ssaview_replace(block, view, &new, 1);
        }
        
        view = ssaview_drop(view, 1);
    }
}