#include "ssa.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../utils.h"

bool ssaview_find(SsaView *view, const SsaOpType type) {
    for (size_t i = view->start; i < view->end; i ++) {
        if (view->block->ops[i].id == type) {
            view->start = i;
            return true;
        }
    }
    
    return false;
}

SsaView ssaview_replace(SsaBlock *viewblock, const SsaView view, const SsaOp *ops, const size_t ops_len) {
    assert(viewblock == view.block);
    
    const size_t new_size = view.block->ops_len - ssaview_len(view) + ops_len;
    SsaOp *new = malloc(sizeof(SsaOp) * new_size);

    memcpy(new, view.block->ops, view.start * sizeof(SsaOp));
    memcpy(new + view.start, ops, ops_len * sizeof(SsaOp));
    memcpy(new + view.start + ops_len, view.block->ops + view.end, (view.block->ops_len - view.end) * sizeof(SsaOp));

    for (size_t i = view.start; i < view.end; i ++)
        ssaop_destroy(&viewblock->ops[i]);
    free(viewblock->ops);
    
    viewblock->ops = new;
    viewblock->ops_len = new_size;
    
    return (SsaView) {
        .block = view.block,
        .start = view.start,
        .end = view.start + ops_len,
    };
}

void ssaview_rename_var(SsaView view, SsaBlock *block, SsaVar old, SsaVar new) {
    assert(block == view.block);
    
    for (size_t i = view.start; i < view.end; i ++) {
        const SsaOp op = block->ops[i];

        for (size_t j = 0; j < op.outs_len; j ++)
            if (op.outs[j].var == old)
                op.outs[j].var = new;
        
        for (size_t j = 0; j < op.params_len; j ++) {
            if (op.params[j].val.type == SSA_VAL_VAR && op.params[j].val.var == old) {
                op.params[j].val.var = new;
            } else if (op.params[j].val.type == SSA_VAL_BLOCK) {
                SsaBlock *child = op.params[j].val.block;
                ssaview_rename_var(ssaview_of_all(child), child, old, new);
            }
        }
    }
}

void ssablock_rename_var(SsaBlock *block, const SsaVar old, const SsaVar new) {
    while (block != NULL) {
        ssaview_rename_var(ssaview_of_all(block), block, old, new);
        block = block->parent;
    }
}

/** You should use `ssablock_root(block)->as_root.vars[id].decl` instead! */
SsaOp *ssablock_finddecl_var(const SsaBlock *block, const SsaVar var) {
    for (size_t i = 0; i < block->ops_len; i ++) {
        SsaOp *op = &block->ops[i];

        for (size_t j = 0; j < op->outs_len; j ++)
            if (op->outs[j].var == var)
                return op;

        for (size_t j = 0; j < op->params_len; j ++) {
            const SsaValue param = op->params[j].val;

            if (param.type == SSA_VAL_BLOCK) {
                for (size_t k = 0; k < param.block->ins_len; k ++)
                    if (param.block->ins[k] == var)
                        return op;

                SsaOp *res = ssablock_finddecl_var(param.block, var);
                if (res != NULL)
                    return res;
            }
        }
    }

    return NULL;
}

SsaOp *ssablock_traverse(SsaView *current) {
    if (ssaview_has_next(*current)) {
        SsaOp *op = &current->block->ops[current->start];
        *current = ssaview_drop(*current, 1);
        return op;
    }

    if (current->block->parent == NULL)
        return NULL;

    *current = ssaview_of_all(current->block->parent);

    return ssablock_traverse(current);
}

// TODO: add boolean to stop traverse
void ssaview_deep_traverse(SsaView top, void (*callback)(SsaOp *op, void *data), void *data) {
    for (size_t i = top.start; i < top.end; i ++) {
        SsaOp *op = &top.block->ops[i];

        for (size_t j = 0; j < op->params_len; j ++)
            if (op->params[j].val.type == SSA_VAL_BLOCK)
                ssaview_deep_traverse(ssaview_of_all(op->params[j].val.block), callback, data);

        callback(op, data);
    }
}

struct ssaview_substitude_var__data {
    SsaBlock *block;
    SsaVar old;
    SsaValue new;
};

static void ssaview_substitude_var__trav(SsaOp *op, void *dataIn) {
    struct ssaview_substitude_var__data *data = dataIn;

    for (size_t i = 0; i < op->params_len; i ++)
        if (op->params[i].val.type == SSA_VAL_VAR && op->params[i].val.var == data->old)
            op->params[i].val = data->new;
}

void ssaview_substitude_var(SsaView view, SsaBlock *block, SsaVar old, SsaValue new) {
    assert(block == view.block);
    struct ssaview_substitude_var__data data = { .block = block, .old = old, .new = new };
    ssaview_deep_traverse(view, ssaview_substitude_var__trav, &data);
}

const SsaBlock *ssablock_root(const SsaBlock *block) {
    while (!block->is_root) {
        block = block->parent;
        if (block == NULL)
            return NULL;
    }
    return block;
}
