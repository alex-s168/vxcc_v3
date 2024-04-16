#include <assert.h>

#include "ssa.h"

#include <stdlib.h>
#include <string.h>

#include "../utils.h"

void ssablock_init(SsaBlock *block, SsaBlock *parent, size_t parent_index) {
    block->parent = parent;
    block->parent_index = parent_index;

    block->is_root = false;

    block->ins = NULL;
    block->ins_len = 0;

    block->ops = NULL;
    block->ops_len = 0;

    block->outs = NULL;
    block->outs_len = 0;

    block->should_free = false;
}

SsaBlock *ssablock_heapalloc(SsaBlock *parent, size_t parent_index) {
    SsaBlock *new = malloc(sizeof(SsaBlock));
    if (new == NULL)
        return NULL;
    ssablock_init(new, parent, parent_index);
    new->should_free = true;
    return new;
}

void ssablock_make_root(SsaBlock *block, const size_t total_vars) {
    assert(block->parent == NULL);

    block->is_root = true;
    block->as_root.vars_len = total_vars;
    block->as_root.vars = malloc(sizeof(*block->as_root.vars) * total_vars);

    for (size_t i = 0; i < total_vars; i ++) {
        SsaOp *decl = ssablock_finddecl_var(block, i);
        // decl can be null!
        block->as_root.vars[i].decl = decl;
    }
}

void ssablock_add_in(SsaBlock *block, const SsaVar var) {
    block->ins = realloc(block->ins, sizeof(SsaVar) * (block->ins_len + 1));
    block->ins[block->ins_len ++] = var;
}

void ssablock_add_op(SsaBlock *block, const SsaOp *op) {
    block->ops = realloc(block->ops, sizeof(SsaOp) * (block->ops_len + 1));
    block->ops[block->ops_len ++] = *op;
}

void ssablock_add_all_op(SsaBlock *dest, const SsaBlock *src) {
    dest->ops = realloc(dest->ops, sizeof(SsaOp) * (dest->ops_len + src->ops_len));
    memcpy(dest->ops + dest->ops_len, src->ops, src->ops_len);
    dest->ops_len += src->ops_len;
}

void ssablock_add_out(SsaBlock *block, SsaVar out) {
    block->outs = realloc(block->outs, sizeof(SsaVar) * (block->outs_len + 1));
    block->outs[block->outs_len ++] = out;
}

void ssablock_destroy(SsaBlock *block) {
    if (block == NULL)
        return;
    free(block->ins);
    for (size_t i = 0; i < block->ops_len; i ++)
        ssaop_destroy(&block->ops[i]);
    free(block->ops);
    free(block->outs);
    if (block->is_root)
        free(block->as_root.vars);
    if (block->should_free)
        free(block);
}

SsaValue *ssaop_param(const SsaOp *op, SsaName name) {
    for (size_t i = 0; i < op->params_len; i ++)
        if (op->params[i].name == name)
            return &op->params[i].val;

    return NULL;
}

void ssanamedvalue_destroy(SsaNamedValue v) {
    if (v.val.type == SSA_VAL_BLOCK)
        ssablock_destroy(v.val.block);
}

void ssaop_init(SsaOp *op, const SsaOpType type, SsaBlock *parent) {
    op->types = NULL;
    op->types_len = 0;

    op->outs = NULL;
    op->outs_len = 0;

    op->params = NULL;
    op->params_len = 0;

    op->id = type;

    op->parent = parent;

    op->states = NULL;
    op->states_len = 0;
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

void ssanamedvalue_rename(SsaNamedValue *value, SsaName newn) {
    value->name = newn;
}

void ssaop_add_param_s(SsaOp *op, SsaName name, const SsaValue val) {
    ssaop_add_param(op, ssanamedvalue_create(name, val));
}

void ssaop_destroy(SsaOp *op) {
    ssaop_remove_params(op);
    free(op->types);
    free(op->outs);
    free(op->states);
}

void ssaop_remove_params(SsaOp *op) {
    for (size_t i = 0; i < op->params_len; i ++)
        ssanamedvalue_destroy(op->params[i]);
    free(op->params);
    op->params = NULL;
    op->params_len = 0;
}


void ssaop_steal_outs(SsaOp *dest, const SsaOp *src) {
    for (size_t i = 0; i < src->outs_len; i ++) {
        ssaop_add_out(dest, src->outs[i].var, src->outs[i].type);
    }
}

void ssaop_remove_out_at(SsaOp *op, const size_t id) {
    memmove(op->outs + id, op->outs + id + 1, sizeof(SsaTypedVar) * (op->outs_len - id - 1));
    op->outs_len --;
}

void ssablock_remove_out_at(SsaBlock *block, size_t id) {
    memmove(block->outs + id, block->outs + id + 1, sizeof(SsaVar) * (block->outs_len - id - 1));
    block->outs_len --;
}

void ssaop_remove_param_at(SsaOp *op, const size_t id) {
    memmove(op->params + id, op->params + id + 1, sizeof(SsaNamedValue) * (op->params_len - id - 1));
    op->params_len --;
}


void ssaop_remove_state_at(SsaOp *op, const size_t id) {
    memmove(op->states + id, op->states + id + 1, sizeof(SsaNamedValue) * (op->states_len - id - 1));
    op->states_len --;
}

SsaValue ssavalue_clone(const SsaValue value) {
    // TODO
    return value;
}

void ssaop_steal_states(SsaOp *dest, const SsaOp *src) {
    free(dest->states);
    dest->states = malloc(sizeof(SsaNamedValue) * src->states_len);
    for (size_t i = 0; i < src->states_len; i ++)
        dest->states[i] = ssavalue_clone(src->states[i]);
    dest->states_len = src->states_len;
}
