#include "../opt.h"
#include <assert.h>
#include <string.h>

/**
 * Find identical computations and join them
 */
void vx_opt_join_compute(view, block)
    vx_IrView view;
    vx_IrBlock *block;
{
    assert(view.block == block);

    for (size_t i = 0; i < block->ops_len; i ++) {
        vx_IrOp *op = &block->ops[i];

        size_t j = i;
        while (j --> 0) {
            vx_IrOp *prev = &block->ops[j];

            if (prev->id != op->id || vx_IrOp_is_volatile(op) || prev->params_len != op->params_len || prev->outs_len == 0)
                continue;

            if (memcmp(prev->params, op->params, sizeof(vx_IrNamedValue) * op->params_len) != 0)
                continue;

            op->id = VX_IR_OP_IMM;
            vx_IrOp_remove_params(op);
            vx_IrOp_add_param_s(op, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VALVAR, .var = prev->outs[0].var });

            break;
        }
    }
}
