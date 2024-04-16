#include "ir.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

SsaOp *irblock_traverse(SsaView *current) {
    if (irview_has_next(*current)) {
        SsaOp *op = &current->block->ops[current->start];
        *current = irview_drop(*current, 1);
        return op;
    }

    if (current->block->parent == NULL)
        return NULL;

    *current = irview_of_all(current->block->parent);

    return irblock_traverse(current);
}

// TODO: add boolean to stop traverse
void irview_deep_traverse(SsaView top, void (*callback)(SsaOp *op, void *data), void *data) {
    for (size_t i = top.start; i < top.end; i ++) {
        SsaOp *op = &top.block->ops[i];

        for (size_t j = 0; j < op->params_len; j ++)
            if (op->params[j].val.type == SSA_VAL_BLOCK)
                irview_deep_traverse(irview_of_all(op->params[j].val.block), callback, data);

        callback(op, data);
    }
}

const SsaBlock *irblock_root(const SsaBlock *block) {
    while (block != NULL && !block->is_root) {
        block = block->parent;
    }
    return block;
}

void irblock_swap_in_at(SsaBlock *block, const size_t a, const size_t b) {
    if (a == b)
        return;

    const SsaVar va = block->ins[a];
    block->ins[a] = block->ins[b];
    block->ins[b] = va;
}

void irblock_swap_out_at(SsaBlock *block, size_t a, size_t b) {
    if (a == b)
        return;

    const SsaVar va = block->outs[a];
    block->outs[a] = block->outs[b];
    block->outs[b] = va;
}

SsaVar irblock_new_var(SsaBlock *block, SsaOp *decl) {
    SsaBlock *root = (SsaBlock *) irblock_root(block);
    root->as_root.vars = realloc(root->as_root.vars, (root->as_root.vars_len + 1) * sizeof(*root->as_root.vars));
    SsaVar new = root->as_root.vars_len ++;
    root->as_root.vars[new].decl = decl;
    return new;
}
