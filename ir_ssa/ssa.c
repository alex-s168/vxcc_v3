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

    ssablock_root(block)->as_root.vars[old].decl = NULL;
    
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
    ssablock_root(block)->as_root.vars[old].decl = NULL;

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
    while (block != NULL && !block->is_root) {
        block = block->parent;
    }
    return block;
}

bool ssablock_var_used(const SsaBlock *block, const SsaVar var) {
    for (size_t i = 0; i < block->outs_len; i++)
        if (block->outs[i] == var)
            return true;

    for (long int i = block->ops_len - 1; i >= 0; i--) {
        const SsaOp *op = &block->ops[i];
        for (size_t j = 0; j < op->params_len; j++) {
            if (op->params[j].val.type == SSA_VAL_BLOCK) {
                if (ssablock_var_used(op->params[j].val.block, var)) {
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

// TODO: search more paths
bool ssaop_anyparam_hasvar(SsaOp *op, SsaVar var) {
    for (size_t i = 0; i < op->params_len; i ++)
        if (op->params[i].val.type == SSA_VAL_VAR && op->params[i].val.var == var)
            return true;
    return false;
}

void ssablock_swap_in(SsaBlock *block, const size_t a, const size_t b) {
    if (a == b)
        return;

    const SsaVar va = block->ins[a];
    block->ins[a] = block->ins[b];
    block->ins[b] = va;
}

void ssablock_swap_out(SsaBlock *block, size_t a, size_t b) {
    if (a == b)
        return;

    const SsaVar va = block->outs[a];
    block->outs[a] = block->outs[b];
    block->outs[b] = va;
}

// TODO: we really need to make state params separate!!
void ssaop_drop_state_param(SsaOp *op, size_t torem) {
    for (size_t i = 0; i < op->params_len; i ++) {
        size_t id;
        if (sscanf(op->params[i].name, "state%zu", &id) == EOF)
            continue;
        
        if (id == torem) {
            ssaop_remove_param(op, id);
            i --;
        } else if (id > torem) {
            id --;
            static char buf[64];
            sprintf(buf, "state%zu", id);
            ssanamedvalue_rename(&op->params[i], buf);
        }
    }
}
