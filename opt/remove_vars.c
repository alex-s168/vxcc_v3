#include "../opt.h"

static bool var_used(SsaBlock *block, SsaVar var) {
    for (size_t i = 0; i < block->outs_len; i++)
        if (block->outs[i] == var)
            return true;

    for (size_t i = 0; i < block->ops_len; i++) {
        SsaOp *op = &block->ops[i];
        for (int j = 0; j < op->params_len; j++) {
            if (op->params[j].val.type == SSA_VAL_BLOCK) {
                if (var_used(op->params[j].val.block, var)) {
                    return true;
                }
            } else if (op->params[j].val.type == SSA_VAL_VAR) {
                if (op->params[j].val.var == var) {
                    return true;
                }
            }
        }
    }

    return false;
}

static void trav(SsaOp *op, void *dataIn) {
    SsaBlock *block = dataIn;

    if (op->outs_len == 0)
        return;

    for (size_t i = 0; i < op->outs_len; i ++)
        if (var_used(block, op->outs[i].var))
            return;

    // no uses, remove 
    ssaop_destroy(op);
    ssaop_init(op, SSA_OP_NOP);
}

void opt_remove_vars(SsaView view, SsaBlock *block) {
    ssaview_deep_traverse(view, trav, block);
}
