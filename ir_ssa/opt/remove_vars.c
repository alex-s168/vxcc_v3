#include "../opt.h"

static bool var_used(SsaBlock *block, SsaVar var) {
    for (size_t i = 0; i < block->outs_len; i++)
        if (block->outs[i] == var)
            return true;

    for (long int i = block->ops_len - 1; i >= 0; i--) {
        const SsaOp *op = &block->ops[i];
        for (size_t j = 0; j < op->params_len; j++) {
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

    // TODO: make smarter for blocks with multiple return!

    for (size_t i = 0; i < op->outs_len; i ++)
        if (var_used(block, op->outs[i].var))
            return;

    // no uses, remove
    for (size_t i = 0; i < op->outs_len; i ++)
        block->as_root.vars[op->outs[i].var].decl = NULL;

    ssaop_destroy(op);
    ssaop_init(op, SSA_OP_NOP);
}

void opt_remove_vars(SsaView view, SsaBlock *block) {
    ssaview_deep_traverse(view, trav, block);
}
