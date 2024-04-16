#include "cir.h"

#include <stdlib.h>
#include <string.h>

#include "../common.h"
#include "../utils.h"

CIRBlock *cirblock_heapalloc(CIRBlock *parent, size_t parent_index) {
    CIRBlock *block = malloc(sizeof(CIRBlock));
    if (block == NULL)
        return NULL;
    cirblock_init(block, parent, parent_index);
    block->should_free = true;
    return block;
}

void cirblock_init(CIRBlock *block, CIRBlock *parent, size_t parent_index) {
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

void cirblock_make_root(CIRBlock *block, const size_t total_vars) {
    block->is_root = true;
    block->as_root.vars_len = total_vars;
}

void cirblock_add_in(CIRBlock *block, CIRVar var) {
    block->ins = realloc(block->ins, sizeof(CIRVar) * (block->ins_len + 1));
    block->ins[block->ins_len ++] = var;
}

void cirblock_add_op(CIRBlock *block, const CIROp *op) {
    block->ops = realloc(block->ops, sizeof(CIROp) * (block->ops_len + 1));
    block->ops[block->ops_len] = *op;
    block->ops[block->ops_len ++].parent = block;
}

void cirblock_add_out(CIRBlock *block, CIRVar out) {
    block->outs = realloc(block->outs, sizeof(CIRVar) * (block->outs_len + 1));
    block->outs[block->outs_len ++] = out;
}

void cirblock_destroy(CIRBlock *block) {
    for (size_t i = 0; i < block->ops_len; i ++)
        cirop_destroy(&block->ops[i]);
    free(block->ins);
    free(block->ops);
    free(block->outs);
}

CIRValue *cirop_param(const CIROp *op, const char *name) {
    for (size_t i = 0; i < op->params_len; i ++)
        if (strcmp(op->params[i].name, name) == 0)
            return &op->params[i].val;

    return NULL;
}

CIRNamedValue cirnamedvalue_create(const char *name, CIRValue v) {
    const size_t len = strlen(name);
    char *new = malloc(len + 1);
    memcpy(new, name, len + 1);

    return (CIRNamedValue) {
        .name = new,
        .val = v,
    };
}

void cirnamedvalue_destroy(CIRNamedValue v) {
    free(v.name);
    if (v.val.type == CIR_VAL_BLOCK && v.val.block->should_free)
        free(v.val.block);
}

void cirop_init(CIROp *op, const CIROpType type) {
    op->types = NULL;
    op->types_len = 0;

    op->outs = NULL;
    op->outs_len = 0;

    op->params = NULL;
    op->params_len = 0;

    op->id = type;

    op->parent = NULL;
}

void cirop_add_type(CIROp *op, CIRType type) {
    op->types = realloc(op->types, sizeof(CIRType) * (op->types_len + 1));
    op->types[op->types_len ++] = type;
}

void cirop_add_out(CIROp *op, CIRVar v, CIRType t) {
    op->outs = realloc(op->outs, sizeof(CIRTypedVar) * (op->outs_len + 1));
    op->outs[op->outs_len ++] = (CIRTypedVar) { .var = v, .type = t };
}

void cirop_add_param(CIROp *op, CIRNamedValue p) {
    op->params = realloc(op->params, sizeof(CIRNamedValue) * (op->params_len + 1));
    op->params[op->params_len ++] = p;
}

void cirop_add_param_s(CIROp *op, const char *name, const CIRValue val) {
    cirop_add_param(op, cirnamedvalue_create(name, val));
}

void cirop_destroy(CIROp *op) {
    cirop_remove_params(op);
    free(op->types);
    free(op->outs);
}

void cirop_remove_params(CIROp *op) {
    for (size_t i = 0; i < op->params_len; i ++)
        cirnamedvalue_destroy(op->params[i]);
    free(op->params);
    op->params = NULL;
    op->params_len = 0;
}

void cirop_steal_all_params_starting_with(CIROp *dest, const CIROp *src, const char *start) {
    for (size_t i = 0; i < src->params_len; i ++) {
        if (!strstarts(src->params[i].name, start))
            continue;

        cirop_add_param(dest, src->params[i]);
    }
}

void cirop_steal_outs(CIROp *dest, const CIROp *src) {
    for (size_t i = 0; i < src->outs_len; i ++) {
        cirop_add_out(dest, src->outs[i].var, src->outs[i].type);
    }
}

void cirblock_add_all_op(CIRBlock *dest, const CIRBlock *src) {
    for (size_t i = 0; i < src->ops_len; i ++) {
        cirblock_add_op(dest, &src->ops[i]);
    }
}
