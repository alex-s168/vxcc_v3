#include "ir.h"

#include <assert.h>
#include <stdlib.h>

// TODO: add boolean to stop traverse
void vx_IrView_deep_traverse(vx_IrView top, void (*callback)(vx_IrOp *op, void *data), void *data) {
    for (size_t i = top.start; i < top.end; i ++) {
        vx_IrOp *op = &top.block->ops[i];

        for (size_t j = 0; j < op->params_len; j ++)
            if (op->params[j].val.type == VX_IR_VAL_BLOCK)
                vx_IrView_deep_traverse(vx_IrView_of_all(op->params[j].val.block), callback, data);

        callback(op, data);
    }
}

const vx_IrBlock *vx_IrBlock_root(const vx_IrBlock *block) {
    while (block != NULL && !block->is_root) {
        block = block->parent;
    }
    return block;
}

void vx_IrBlock_swap_in_at(vx_IrBlock *block, const size_t a, const size_t b) {
    if (a == b)
        return;

    const vx_IrVar va = block->ins[a];
    block->ins[a] = block->ins[b];
    block->ins[b] = va;
}

void vx_IrBlock_swap_out_at(vx_IrBlock *block, size_t a, size_t b) {
    if (a == b)
        return;

    const vx_IrVar va = block->outs[a];
    block->outs[a] = block->outs[b];
    block->outs[b] = va;
}

vx_IrVar vx_IrBlock_new_var(vx_IrBlock *block, vx_IrOp *decl) {
    assert(block != NULL);
    assert(decl != NULL);
    vx_IrBlock *root = (vx_IrBlock *) vx_IrBlock_root(block);
    assert(root != NULL);
    root->as_root.vars = realloc(root->as_root.vars, (root->as_root.vars_len + 1) * sizeof(*root->as_root.vars));
    vx_IrVar new = root->as_root.vars_len ++;
    root->as_root.vars[new].decl = decl;
    return new;
}
