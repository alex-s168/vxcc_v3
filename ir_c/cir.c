#include "cir.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../common.h"

const CIRBlock *cirblock_root(const CIRBlock *block) {
    while (!block->is_root) {
        block = block->parent;
        if (block == NULL)
            return NULL;
    }
    return block;
}

void cirblock_rename_var_after(CIRBlock *block, size_t i, const CIRVar old, const CIRVar newd) {
    for (; i < block->ops_len; i ++) {
        const CIROp *op = &block->ops[i];

        for (size_t j = 0; j < op->outs_len; j ++)
            if (op->outs[j].var == old)
                op->outs[j].var = newd;

        for (size_t j = 0; j < op->params_len; j ++) {
            CIRValue *v = &op->params[j].val;

            if (v->type == CIR_VAL_VAR && v->var == old)
                v->var = newd;

            else if (v->type == CIR_VAL_BLOCK)
                cirblock_rename_var(v->block, old, newd);
        }
    }
}

CIRVar cirblock_new_var(CIRBlock *block) {
    block = (CIRBlock *) cirblock_root(block);
    assert(block != NULL);

    return block->as_root.vars_len ++;
}

CIROp *cirblock_inside_out_vardecl_before(const CIRBlock *block, const CIRVar var, size_t before) {
    while (before --> 0) {
        CIROp *op = &block->ops[before];

        for (size_t i = 0; i < op->outs_len; i ++)
            if (op->outs[i].var == var)
                return op;
    }

    if (block->parent == NULL)
        return NULL;

    return cirblock_inside_out_vardecl_before(block->parent, var, block->parent_index);
}
