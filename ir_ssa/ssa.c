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

    if (view.start == 0)
        for (size_t j = 0; j < block->ins_len; j ++)
            if (block->ins[j] == old)
                block->ins[j] = new;

    if (view.end == block->ops_len)
        for (size_t j = 0; j < block->outs_len; j ++)
            if (block->outs[j] == old)
                block->outs[j] = new;
    
    ssablock_root(block)->as_root.vars[old].decl = NULL;
    
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
                ssaview_rename_var(ssaview_of_all(child), child, old, new);
            }
        }
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

void ssablock_swap_in_at(SsaBlock *block, const size_t a, const size_t b) {
    if (a == b)
        return;

    const SsaVar va = block->ins[a];
    block->ins[a] = block->ins[b];
    block->ins[b] = va;
}

void ssablock_swap_out_at(SsaBlock *block, size_t a, size_t b) {
    if (a == b)
        return;

    const SsaVar va = block->outs[a];
    block->outs[a] = block->outs[b];
    block->outs[b] = va;
}

SsaOp *ssablock_inside_out_vardecl_before(const SsaBlock *block, const SsaVar var, size_t before) {
    while (before --> 0) {
        SsaOp *op = &block->ops[before];

        for (size_t i = 0; i < op->outs_len; i ++)
            if (op->outs[i].var == var)
                return op;
    }

    if (block->parent == NULL)
        return NULL;

    return ssablock_inside_out_vardecl_before(block->parent, var, block->parent_index);
}

SsaVar ssablock_new_var(SsaBlock *block, SsaOp *decl) {
    SsaBlock *root = (SsaBlock *) ssablock_root(block);
    root->as_root.vars = realloc(root->as_root.vars, (root->as_root.vars_len + 1) * sizeof(*root->as_root.vars));
    SsaVar new = root->as_root.vars_len ++;
    root->as_root.vars[new].decl = decl;
    return new;
}

bool ssaop_is_pure(SsaOp *op) {
    switch (op->id) {
        case SSA_OP_NOP:
        case SSA_OP_IMM:
        case SSA_OP_REINTERPRET:
        case SSA_OP_ZEROEXT:
        case SSA_OP_SIGNEXT:
        case SSA_OP_TOFLT:
        case SSA_OP_FROMFLT:
        case SSA_OP_BITCAST:
        case SSA_OP_ADD:
        case SSA_OP_SUB:
        case SSA_OP_MUL:
        case SSA_OP_DIV:
        case SSA_OP_MOD:
        case SSA_OP_GT:
        case SSA_OP_GTE:
        case SSA_OP_LT:
        case SSA_OP_LTE:
        case SSA_OP_EQ:
        case SSA_OP_NEQ:
        case SSA_OP_NOT:
        case SSA_OP_AND:
        case SSA_OP_OR:
        case SSA_OP_BITWISE_NOT:
        case SSA_OP_BITWISE_AND:
        case SSA_OP_BITIWSE_OR:
        case SSA_OP_SHL:
        case SSA_OP_SHR:
        case SSA_OP_FOR:
            return true;

        case SSA_OP_REPEAT:
        case CIR_OP_CFOR:
        case SSA_OP_IF:
        case SSA_OP_FLATTEN_PLEASE:
        case SSA_OP_INFINITE:
        case SSA_OP_WHILE:
        case SSA_OP_FOREACH:
        case SSA_OP_FOREACH_UNTIL:
            for (size_t i = 0; i < op->params_len; i ++)
                if (op->params[i].val.type == SSA_VAL_BLOCK)
                    for (size_t j = 0; j < op->params[i].val.block->ops_len; j ++)
                        if (!ssaop_is_pure(&op->params[i].val.block->ops[j]))
                            return false;
            return true;

        default:
            return false;
    }
}
