#include "ir.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

vx_IrView vx_IrView_replace(vx_IrBlock *viewblock, const vx_IrView view, const vx_IrOp *ops, const size_t ops_len) {
    assert(viewblock == view.block);

    const size_t new_size = view.block->ops_len - vx_IrView_len(view) + ops_len;
    vx_IrOp *new = malloc(sizeof(vx_IrOp) * new_size);

    memcpy(new, view.block->ops, view.start * sizeof(vx_IrOp));
    memcpy(new + view.start, ops, ops_len * sizeof(vx_IrOp));
    memcpy(new + view.start + ops_len, view.block->ops + view.end, (view.block->ops_len - view.end) * sizeof(vx_IrOp));

    for (size_t i = view.start; i < view.end; i ++)
        vx_IrOp_destroy(&viewblock->ops[i]);
    free(viewblock->ops);

    viewblock->ops = new;
    viewblock->ops_len = new_size;

    return (vx_IrView) {
        .block = view.block,
        .start = view.start,
        .end = view.start + ops_len,
    };
}

void vx_IrView_rename_var(vx_IrView view, vx_IrBlock *block, vx_IrVar old, vx_IrVar new) {
    assert(block == view.block);

    if (view.start == 0)
        for (size_t j = 0; j < block->ins_len; j ++)
            if (block->ins[j] == old)
                block->ins[j] = new;

    if (view.end >= block->ops_len)
        for (size_t j = 0; j < block->outs_len; j ++)
            if (block->outs[j] == old)
                block->outs[j] = new;

    vx_IrBlock_root(block)->as_root.vars[old].decl_parent = NULL;

    for (size_t i = view.start; i < view.end; i ++) {
        const vx_IrOp op = block->ops[i];

        for (size_t j = 0; j < op.outs_len; j ++)
            if (op.outs[j].var == old)
                op.outs[j].var = new;

        for (size_t j = 0; j < op.params_len; j ++) {
            if (op.params[j].val.type == VX_IR_VAL_VAR && op.params[j].val.var == old) {
                op.params[j].val.var = new;
            }
            else if (op.params[j].val.type == VX_IR_VAL_BLOCK) {
                vx_IrBlock *child = op.params[j].val.block;
                vx_IrView_rename_var(vx_IrView_of_all(child), child, old, new);
            }
        }
    }
}

struct vx_IrView_substitute_var__data {
    vx_IrBlock *block;
    vx_IrVar old;
    vx_IrValue new;
};

static void vx_IrView_substitute_var__trav(vx_IrOp *op, void *dataIn) {
    struct vx_IrView_substitute_var__data *data = dataIn;

    for (size_t i = 0; i < op->params_len; i ++)
        if (op->params[i].val.type == VX_IR_VAL_VAR && op->params[i].val.var == data->old)
            op->params[i].val = data->new;
}

void vx_IrView_substitute_var(vx_IrView view, vx_IrBlock *block, vx_IrVar old, vx_IrValue new) {
    assert(block == view.block);
    struct vx_IrView_substitute_var__data data = { .block = block, .old = old, .new = new };
    vx_IrView_deep_traverse(view, vx_IrView_substitute_var__trav, &data);
}
