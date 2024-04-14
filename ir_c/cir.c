#include "cir.h"

#include <stdlib.h>
#include <string.h>

#include "../common.h"
#include "../utils.h"

/** You should use `cirblock_root(block)->as_root.vars[id].decl` instead! */
CIROp *cirblock_finddecl_var(const CIRBlock *block, const CIRVar var) {
    for (size_t i = 0; i < block->ops_len; i ++) {
        CIROp *op = &block->ops[i];

        for (size_t j = 0; j < op->outs_len; j ++)
            if (op->outs[j].var == var)
                return op;

        for (size_t j = 0; j < op->params_len; j ++) {
            const CIRValue param = op->params[j].val;

            if (param.type == CIR_VAL_BLOCK) {
                for (size_t k = 0; k < param.block->ins_len; k ++)
                    if (param.block->ins[k] == var)
                        return op;

                CIROp *res = cirblock_finddecl_var(param.block, var);
                if (res != NULL)
                    return res;
            }
        }
    }

    return NULL;
}

const CIRBlock *cirblock_root(const CIRBlock *block) {
    while (!block->is_root) {
        block = block->parent;
        if (block == NULL)
            return NULL;
    }
    return block;
}
