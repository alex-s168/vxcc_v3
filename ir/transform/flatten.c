#include <stdlib.h>

#include "../ir.h"

static void flatten_into(SsaOp *ops, size_t ops_len, SsaBlock *dest) {
    for (size_t i = 0; i < ops_len; i ++) {
        if (ops[i].id == SSA_OP_FLATTEN_PLEASE) {
            const SsaBlock *child = irop_param(&ops[i], SSA_NAME_BLOCK)->block;
            flatten_into(child->ops, child->ops_len, dest);
        }
        else {
            irblock_add_op(dest, &ops[i]);
        }
    }
}

void irblock_flatten(SsaBlock *block) {
    SsaOp *old_ops = block->ops;
    block->ops = NULL;
    const size_t old_ops_len = block->ops_len;
    block->ops_len = 0;
    flatten_into(old_ops, old_ops_len, block);
    free(old_ops);
}
