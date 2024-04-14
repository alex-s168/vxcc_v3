#include "ssa.h"

#include <stdlib.h>
#include <string.h>

#include "../utils.h"

void ssablock_init(SsaBlock *block, SsaBlock *parent) {
    block->parent = parent;

    block->is_root = false;

    block->ins = NULL;
    block->ins_len = 0;

    block->ops = NULL;
    block->ops_len = 0;

    block->outs = NULL;
    block->outs_len = 0;
}

void ssablock_make_root(SsaBlock *block, const size_t total_vars) {
    block->is_root = true;
    block->as_root.vars_len = total_vars;
    block->as_root.vars = malloc(sizeof(*block->as_root.vars) * total_vars);

    for (size_t i = 0; i < total_vars; i ++) {
        SsaOp *decl = ssablock_finddecl_var(block, i);
        // decl can be null!
        block->as_root.vars[i].decl = decl;
    }
}

void ssablock_add_in(SsaBlock *block, SsaVar var) {
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
    free(block->ins);
    free(block->ops);
    free(block->outs);
    if (block->is_root)
        free(block->as_root.vars);
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
    ssaop_remove_params(op);
    free(op->types);
    free(op->outs);
}

void ssaop_remove_params(SsaOp *op) {
    for (size_t i = 0; i < op->params_len; i ++)
        ssanamedvalue_destroy(op->params[i]);
    free(op->params);
    op->params = NULL;
    op->params_len = 0;
}

void ssaop_steal_all_params_starting_with(SsaOp *dest, const SsaOp *src, const char *start) {
    for (size_t i = 0; i < src->params_len; i ++) {
        if (!strstarts(src->params[i].name, start))
            continue;

        ssaop_add_param(dest, src->params[i]);
    }
}

void ssaop_steal_outs(SsaOp *dest, const SsaOp *src) {
    for (size_t i = 0; i < src->outs_len; i ++) {
        ssaop_add_out(dest, src->outs[i].var, src->outs[i].type);
    }
}

void ssaop_remove_out(SsaOp *op, const size_t id) {
    memmove(op->outs + id, op->outs + id + 1, sizeof(SsaTypedVar) * (op->outs_len - id - 1));
    op->outs_len --;
}

void ssablock_remove_out(SsaBlock *block, size_t id) {
    memmove(block->outs + id, block->outs + id + 1, sizeof(SsaVar) * (block->outs_len - id - 1));
    block->outs_len --;
}
