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
void vx_opt_reduce_loops(vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->id != VX_IR_OP_IF)
            continue;

        if (op->id == VX_IR_OP_WHILE && op->outs_len > 0) { // "fast path" for when we don't even have states
            vx_IrBlock **bdo = &vx_IrOp_param(op, VX_IR_NAME_LOOP_DO)->block;
            vx_IrBlock *newdo = *bdo;
            vx_IrBlock **bcond = &vx_IrOp_param(op, VX_IR_NAME_COND)->block;
            vx_IrBlock *newcond = *bcond;

            // TODO: if we detect multiple counters here, chose the one which is single present in the condition
            
            vx_IrOp *incOp = NULL;
            size_t stateId;
            struct IrStaticIncrement si;
            for (stateId = 0; stateId < op->outs_len; stateId ++) {
                bool found = false;
                for (vx_IrOp *incOp = newdo->first; incOp; incOp = incOp->next) {
                    si = vx_IrOp_detectStaticIncrement(incOp);
                    if (si.detected &&
                        incOp->outs[0].var == op->outs[stateId].var &&
                        si.var == newcond->ins[stateId].var)
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

            const vx_IrValue init = op->args[stateId];

            // counter needs to be at pos 0 oviously
            vx_IrBlock_swapInAt(newcond, stateId, 0);
            vx_IrBlock_swapOutAt(newcond, stateId, 0);
            vx_IrOp_removeArgAt(op, stateId);

            vx_IrOp new;
            vx_IrOp_init(&new, VX_IR_OP_FOR, block);
            vx_IrOp_addParam_s(&new, VX_IR_NAME_LOOP_START, init);
            vx_IrOp_addParam_s(&new, VX_IR_NAME_LOOP_STRIDE, VX_IR_VALUE_IMM_INT(si.by));
            vx_IrOp_stealArgs(&new, op);
            vx_IrOp_addParam_s(&new, VX_IR_NAME_COND, VX_IR_VALUE_BLK(newcond));
            vx_IrOp_addParam_s(&new, VX_IR_NAME_LOOP_DO, VX_IR_VALUE_BLK(newdo));

            vx_IrOp_remove(incOp);

            vx_IrOp_destroy(op);
            *op = new;
        } 
    }
}
