#include <assert.h>

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

void opt_remove_vars(SsaBlock *block) {
    assert(block->is_root);

    for (size_t i = 0; i < block->as_root.vars_len; i ++) {
        SsaOp *decl = block->as_root.vars[i].decl;
        if (decl == NULL)
            continue;

        // TODO: optimize away states and similar

        // we can't optimize away loop counters and similar
        bool in_outs = false;
        for (size_t j = 0; j < decl->outs_len; j ++) {
            if (decl->outs[j].var == i) {
                in_outs = true;
                break;
            }
        }
        if (!in_outs)
            continue;

        // we can't optimize away if we need other results from the operation
        bool can_rem = true;
        for (size_t j = 0; j < decl->outs_len; j ++) {
            if (var_used(block, decl->outs[j].var)) {
                can_rem = false;
                break;
            }
        }

        if (!can_rem)
            continue;

        // in-place remove
        ssaop_destroy(decl);
        ssaop_init(decl, SSA_OP_NOP);

        for (size_t j = 0; j < decl->outs_len; j ++)
            block->as_root.vars[decl->outs[j].var].decl = NULL;
    }
}
