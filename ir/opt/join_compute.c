#include "../opt.h"
#include <assert.h>
#include <string.h>

/**
 * Find identical computations and join them
 */
void vx_opt_join_compute(vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        for (vx_IrOp *prev = block->first; prev != op; prev = prev->next) {
            if (prev->id != op->id || vx_IrOp_is_volatile(op) || prev->params_len != op->params_len || prev->outs_len == 0)
                continue;

            if (memcmp(prev->params, op->params, sizeof(vx_IrNamedValue) * op->params_len) != 0)
                continue;

            op->id = VX_IR_OP_IMM;
            vx_IrOp_remove_params(op);
            vx_IrOp_add_param_s(op, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = prev->outs[0].var });

            break;
        }
    }
}
