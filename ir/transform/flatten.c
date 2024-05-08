#include <stdlib.h>

#include "../ir.h"

static void flatten_into(vx_IrOp *ops,
                         size_t ops_len,
                         vx_IrBlock *dest)
{
    for (size_t i = 0; i < ops_len; i ++) {
        if (ops[i].id == VX_IR_OP_FLATTEN_PLEASE) {
            const vx_IrBlock *child = vx_IrOp_param(&ops[i], VX_IR_NAME_BLOCK)->block;
            flatten_into(child->ops, child->ops_len, dest);
        }
        else {
            vx_IrBlock_add_op(dest, &ops[i]);
        }
    }
}

void vx_IrBlock_flatten(vx_IrBlock *block)
{
    vx_IrOp *old_ops = block->ops;
    block->ops = NULL;
    const size_t old_ops_len = block->ops_len;
    block->ops_len = 0;
    flatten_into(old_ops, old_ops_len, block);
    free(old_ops);
}
