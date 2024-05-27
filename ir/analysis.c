#include <assert.h>

#include "ir.h"

bool vx_IrView_find(vx_IrView *view,
                    const vx_IrOpType type)
{
    for (size_t i = view->start; i < view->end; i ++) {
        if (view->block->ops[i].id == type) {
            view->start = i;
            return true;
        }
    }

    return false;
}

/** You should use `vx_IrBlock_root(block)->as_root.vars[id].decl` instead! */
vx_IrOp *vx_IrBlock_find_var_decl(const vx_IrBlock *block,
                                  const vx_IrVar var)
{
    for (size_t i = 0; i < block->ops_len; i ++) {
        vx_IrOp *op = &block->ops[i];

        for (size_t j = 0; j < op->outs_len; j ++)
            if (op->outs[j].var == var)
                return op;

        for (size_t j = 0; j < op->params_len; j ++) {
            const vx_IrValue param = op->params[j].val;

            if (param.type == VX_IR_VAL_BLOCK) {
                for (size_t k = 0; k < param.block->ins_len; k ++)
                    if (param.block->ins[k] == var)
                        return op;

                vx_IrOp *res = vx_IrBlock_find_var_decl(param.block, var);
                if (res != NULL)
                    return res;
            }
        }
    }

    return NULL;
}

bool vx_IrOp_var_used(const vx_IrOp *op, vx_IrVar var) {
    for (size_t j = 0; j < op->outs_len; j++)
        if (op->outs[j].var == var)
            return true;
    for (size_t j = 0; j < op->params_len; j++) {
        if (op->params[j].val.type == VX_IR_VAL_BLOCK) {
            if (vx_IrBlock_var_used(op->params[j].val.block, var)) {
                return true;
            }
        } else if (op->params[j].val.type == VX_IR_VAL_VAR) {
            if (op->params[j].val.var == var) {
                return true;
            }
        }
    }
    return false;
}

bool vx_IrBlock_var_used(const vx_IrBlock *block,
                         const vx_IrVar var)
{
    for (size_t i = 0; i < block->outs_len; i++)
        if (block->outs[i] == var)
            return true;

    for (long int i = block->ops_len - 1; i >= 0; i--) {
        const vx_IrOp *op = &block->ops[i];
        if (vx_IrOp_var_used(op, var))
            return true;
    }

    return false;
}

vx_IrOp *vx_IrBlock_inside_out_vardecl_before(const vx_IrBlock *block,
                                              const vx_IrVar var,
                                              size_t before)
{
    while (before --> 0) {
        vx_IrOp *op = &block->ops[before];

        for (size_t i = 0; i < op->outs_len; i ++)
            if (op->outs[i].var == var)
                return op;
    }

    if (block->parent == NULL)
        return NULL;

    return vx_IrBlock_inside_out_vardecl_before(block->parent, var, block->parent_index);
}

bool vx_IrOp_is_volatile(vx_IrOp *op)
{
    switch (op->id) {
        case VX_IR_OP_NOP:
        case VX_IR_OP_IMM:
        case VX_IR_OP_REINTERPRET:
        case VX_IR_OP_ZEROEXT:
        case VX_IR_OP_SIGNEXT:
        case VX_IR_OP_TOFLT:
        case VX_IR_OP_FROMFLT:
        case VX_IR_OP_BITCAST:
        case VX_IR_OP_ADD:
        case VX_IR_OP_SUB:
        case VX_IR_OP_MUL:
        case VX_IR_OP_DIV:
        case VX_IR_OP_MOD:
        case VX_IR_OP_GT:
        case VX_IR_OP_GTE:
        case VX_IR_OP_LT:
        case VX_IR_OP_LTE:
        case VX_IR_OP_EQ:
        case VX_IR_OP_NEQ:
        case VX_IR_OP_NOT:
        case VX_IR_OP_AND:
        case VX_IR_OP_OR:
        case VX_IR_OP_BITWISE_NOT:
        case VX_IR_OP_BITWISE_AND:
        case VX_IR_OP_BITIWSE_OR:
        case VX_IR_OP_SHL:
        case VX_IR_OP_SHR:
        case VX_IR_OP_FOR:
        case VX_IR_OP_LOAD:
        case VX_IR_OP_CMOV:
            return false;

        case VX_IR_OP_STORE:
        case VX_IR_OP_PLACE: 
            return true;

        case VX_IR_OP_LOAD_VOLATILE:
        case VX_IR_OP_STORE_VOLATILE:
            return true;
        
        case VX_IR_OP_REPEAT:
        case VX_CIR_OP_CFOR:
        case VX_IR_OP_IF:
        case VX_IR_OP_FLATTEN_PLEASE:
        case VX_IR_OP_INFINITE:
        case VX_IR_OP_WHILE:
        case VX_IR_OP_FOREACH:
        case VX_IR_OP_FOREACH_UNTIL:
            for (size_t i = 0; i < op->params_len; i ++)
                if (op->params[i].val.type == VX_IR_VAL_BLOCK)
                    if (vx_IrBlock_is_volatile(op->params[i].val.block))
                        return true;
            return false;

        default:
            assert(false);
            return false; // never reached
    }
}

bool vx_IrBlock_is_volatile(const vx_IrBlock *block)
{
    for (size_t j = 0; j < block->ops_len; j ++)
        if (vx_IrOp_is_volatile(&block->ops[j]))
            return true;
    return false;
}

vx_IrType *vx_IrBlock_typeof_var(vx_IrBlock *block, vx_IrVar var) {
    vx_IrOp *decl = vx_IrBlock_root_get_var_decl(vx_IrBlock_root(block), var);
    if (decl == NULL)
        goto warn;

    for (size_t i = 0; i < decl->outs_len; i ++)
        if (decl->outs[i].var == var)
            return decl->outs[i].type;

warn:
    fprintf(stderr, "VARIABLE %%%zu DECL ERROR (typeof_var)\n", var);
    return NULL;
}

static size_t cost_lut[VX_IR_OP____END] = {
    [VX_IR_OP_NOP] = 0,
    [VX_IR_OP_IMM] = 1,
    [VX_IR_OP_FLATTEN_PLEASE] = 0,
    [VX_IR_OP_REINTERPRET] = 0,
    [VX_IR_OP_ZEROEXT] = 1,
    [VX_IR_OP_SIGNEXT] = 1,
    [VX_IR_OP_TOFLT] = 1,
    [VX_IR_OP_FROMFLT] = 1,
    [VX_IR_OP_BITCAST] = 1,
    [VX_IR_OP_LOAD] = 1,
    [VX_IR_OP_LOAD_VOLATILE] = 1,
    [VX_IR_OP_STORE] = 1,
    [VX_IR_OP_STORE_VOLATILE] = 1,
    [VX_IR_OP_PLACE] = 0,
    [VX_IR_OP_ADD] = 1,
    [VX_IR_OP_SUB] = 1,
    [VX_IR_OP_MUL] = 1,
    [VX_IR_OP_DIV] = 1,
    [VX_IR_OP_MOD] = 1,
    [VX_IR_OP_GT] = 1,
    [VX_IR_OP_GTE] = 1,
    [VX_IR_OP_LT] = 1,
    [VX_IR_OP_LTE] = 1,
    [VX_IR_OP_EQ] = 1,
    [VX_IR_OP_NEQ] = 1,
    [VX_IR_OP_NOT] = 1,
    [VX_IR_OP_AND] = 1,
    [VX_IR_OP_OR] = 1,
    [VX_IR_OP_BITWISE_NOT] = 1,
    [VX_IR_OP_BITWISE_AND] = 1,
    [VX_IR_OP_BITIWSE_OR] = 1,
    [VX_IR_OP_SHL] = 1,
    [VX_IR_OP_FOR] = 1,
    [VX_IR_OP_INFINITE] = 1,
    [VX_IR_OP_WHILE] = 1,
    [VX_IR_OP_CONTINUE] = 1,
    [VX_IR_OP_BREAK] = 1,
    [VX_IR_OP_FOREACH] = 2,
    [VX_IR_OP_FOREACH_UNTIL] = 2,
    [VX_IR_OP_REPEAT] = 1,
    [VX_CIR_OP_CFOR] = 2,
    [VX_IR_OP_IF] = 1,
    [VX_IR_OP_CMOV] = 1,
    [VX_LIR_OP_LABEL] = 1,
    [VX_LIR_GOTO] = 1,
    [VX_LIR_COND] = 1,
};

size_t vx_IrOp_inline_cost(vx_IrOp *op) {
    size_t total = 0;

    for (size_t i = 0; i < op->params_len; i ++)
        if (op->params[i].val.type == VX_IR_NAME_BLOCK)
            total += vx_IrBlock_inline_cost(op->params[i].val.block);

    total += cost_lut[op->id];

    return total;
}

size_t vx_IrBlock_inline_cost(const vx_IrBlock *block) {
    size_t total = 0;
    for (size_t i = 0; i < block->ops_len; i ++) {
        vx_IrOp *op = &block->ops[i];
        total += vx_IrOp_inline_cost(op);
    }
    return total;
}
