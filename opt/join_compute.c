#include "../opt.h"
#include <assert.h>
#include <string.h>

void opt_join_compute(SsaView view, SsaBlock *block) {
    assert(view.block == block);

    for (size_t i = 0; i < block->ops_len; i ++) {
        SsaOp *op = &block->ops[i];

        size_t j = i;
        while (j --> 0) {
            SsaOp *prev = &block->ops[j];

            if (prev->id != op->id)
                continue;

            if (prev->params_len != op->params_len)
                continue;

            if (prev->outs_len == 0)
                continue;

            if (memcmp(prev->params, op->params, sizeof(SsaNamedValue) * op->params_len) != 0)
                continue;

            op->id = SSA_OP_IMM;
            ssaop_remove_params(op);
            ssaop_add_param_s(op, "val", (SsaValue) { .type = SSA_VAL_VAR, .var = prev->outs[0].var });

            break;
        }
    }
}
