#include "ir.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

SsaView irview_replace(SsaBlock *viewblock, const SsaView view, const SsaOp *ops, const size_t ops_len) {
    assert(viewblock == view.block);

    const size_t new_size = view.block->ops_len - irview_len(view) + ops_len;
    SsaOp *new = malloc(sizeof(SsaOp) * new_size);

    memcpy(new, view.block->ops, view.start * sizeof(SsaOp));
    memcpy(new + view.start, ops, ops_len * sizeof(SsaOp));
    memcpy(new + view.start + ops_len, view.block->ops + view.end, (view.block->ops_len - view.end) * sizeof(SsaOp));

    for (size_t i = view.start; i < view.end; i ++)
        irop_destroy(&viewblock->ops[i]);
    free(viewblock->ops);

    viewblock->ops = new;
    viewblock->ops_len = new_size;

    return (SsaView) {
        .block = view.block,
        .start = view.start,
        .end = view.start + ops_len,
    };
}

void irview_rename_var(SsaView view, SsaBlock *block, SsaVar old, SsaVar new) {
    assert(block == view.block);

    if (view.start == 0)
        for (size_t j = 0; j < block->ins_len; j ++)
            if (block->ins[j] == old)
                block->ins[j] = new;

    if (view.end == block->ops_len)
        for (size_t j = 0; j < block->outs_len; j ++)
            if (block->outs[j] == old)
                block->outs[j] = new;

    irblock_root(block)->as_root.vars[old].decl = NULL;

    for (size_t i = view.start; i < view.end; i ++) {
        const SsaOp op = block->ops[i];

        for (size_t j = 0; j < op.outs_len; j ++)
            if (op.outs[j].var == old)
                op.outs[j].var = new;

        for (size_t j = 0; j < op.params_len; j ++) {
            if (op.params[j].val.type == SSA_VAL_VAR && op.params[j].val.var == old) {
                op.params[j].val.var = new;
            }
            else if (op.params[j].val.type == SSA_VAL_BLOCK) {
                SsaBlock *child = op.params[j].val.block;
                irview_rename_var(irview_of_all(child), child, old, new);
            }
        }
    }
}

struct irview_substitude_var__data {
    SsaBlock *block;
    SsaVar old;
    SsaValue new;
};

static void irview_substitude_var__trav(SsaOp *op, void *dataIn) {
    struct irview_substitude_var__data *data = dataIn;

    for (size_t i = 0; i < op->params_len; i ++)
        if (op->params[i].val.type == SSA_VAL_VAR && op->params[i].val.var == data->old)
            op->params[i].val = data->new;
}

void irview_substitude_var(SsaView view, SsaBlock *block, SsaVar old, SsaValue new) {
    assert(block == view.block);
    struct irview_substitude_var__data data = { .block = block, .old = old, .new = new };
    irview_deep_traverse(view, irview_substitude_var__trav, &data);
}
