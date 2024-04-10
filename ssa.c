#include "ssa.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void ssablock_init(SsaBlock *block, SsaBlock *parent) {
    block->parent = parent;
    
    block->ins = NULL;
    block->ins_len = 0;

    block->ops = NULL;
    block->ops_len = 0;

    block->outs = NULL;
    block->outs_len = 0;
}

void ssablock_add_in(SsaBlock *block, SsaVar var) {
    block->ins = realloc(block->ins, sizeof(SsaVar) * (block->ins_len + 1));
    block->ins[block->ins_len ++] = var;
}

void ssablock_add_op(SsaBlock *block, const SsaOp *op) {
    block->ops = realloc(block->ops, sizeof(SsaOp) * (block->ops_len + 1));
    block->ops[block->ops_len ++] = *op;
}

void ssablock_add_out(SsaBlock *block, SsaVar out) {
    block->outs = realloc(block->outs, sizeof(SsaVar) * (block->outs_len + 1));
    block->outs[block->outs_len ++] = out;
}

void ssablock_destroy(SsaBlock *block) {
    free(block->ins);
    free(block->ops);
    free(block->outs);
}

SsaValue *ssaop_param(const SsaOp *op, const char *name) {
    for (size_t i = 0; i < op->params_len; i ++)
        if (strcmp(op->params[i].name, name) == 0)
            return &op->params[i].val;
    
    return NULL;
}

SsaNamedValue ssanamedvalue_create(const char *name, SsaValue v) {
    const size_t len = strlen(name);
    char *new = malloc(len + 1);
    memcpy(new, name, len + 1);
    
    return (SsaNamedValue) {
        .name = new,
        .val = v,
    };
}

void ssanamedvalue_destroy(SsaNamedValue v) {
    free(v.name);
}

void ssaop_init(SsaOp *op, const SsaOpType type) {
    op->types = NULL;
    op->types_len = 0;
    
    op->outs = NULL;
    op->outs_len = 0;

    op->params = NULL;
    op->params_len = 0;

    op->id = type;
}

void ssaop_add_type(SsaOp *op, SsaType type) {
    op->types = realloc(op->types, sizeof(SsaType) * (op->types_len + 1));
    op->types[op->types_len ++] = type;
}

void ssaop_add_out(SsaOp *op, SsaVar v, SsaType t) {
    op->outs = realloc(op->outs, sizeof(SsaTypedVar) * (op->outs_len + 1));
    op->outs[op->outs_len ++] = (SsaTypedVar) { .var = v, .type = t };
}

void ssaop_add_param(SsaOp *op, SsaNamedValue p) {
    op->params = realloc(op->params, sizeof(SsaNamedValue) * (op->params_len + 1));
    op->params[op->params_len ++] = p;
}

void ssaop_add_param_s(SsaOp *op, const char *name, const SsaValue val) {
    ssaop_add_param(op, ssanamedvalue_create(name, val));
}

void ssaop_destroy(SsaOp *op) {
    for (size_t i = 0; i < op->params_len; i ++)
        ssanamedvalue_destroy(op->params[i]);

    free(op->params);
    free(op->types);
    free(op->outs);
}

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

    memcpy(new, view.block->ops, view.start);
    memcpy(new + view.start, ops, ops_len);
    memcpy(new + view.start + ops_len, view.block->ops + view.end, view.block->ops_len - view.end);

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

SsaOp *ssablock_finddecl_var(const SsaBlock *block, SsaVar var) {
    while (block != NULL) {
        for (size_t i = 0; i < block->ops_len; i ++) {
            const SsaOp op = block->ops[i];

            for (size_t j = 0; j < op.outs_len; j ++)
                if (op.outs[j].var == var)
                    return &block->ops[i];
        }
        block = block->parent;
    }

    return NULL;
}

bool ssablock_staticeval_var(const SsaBlock *block, SsaVar var, SsaValue *dest) {
    const SsaOp *decl = ssablock_finddecl_var(block, var);
    if (decl == NULL)
        return false;

    if (decl->id == SSA_OP_IMM) {
        const SsaValue *value = ssaop_param(decl, "val");
        if (value == NULL)
            return false;

        if (value->type == SSA_VAL_VAR)
            return ssablock_staticeval_var(block, value->var, dest);

        *dest = *value;
        return true;
    }
    
    return false;
}