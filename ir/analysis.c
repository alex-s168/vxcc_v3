#include "ir.h"

bool irview_find(SsaView *view, const SsaOpType type) {
    for (size_t i = view->start; i < view->end; i ++) {
        if (view->block->ops[i].id == type) {
            view->start = i;
            return true;
        }
    }

    return false;
}

/** You should use `irblock_root(block)->as_root.vars[id].decl` instead! */
SsaOp *irblock_finddecl_var(const SsaBlock *block, const SsaVar var) {
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

                SsaOp *res = irblock_finddecl_var(param.block, var);
                if (res != NULL)
                    return res;
            }
        }
    }

    return NULL;
}


bool irblock_var_used(const SsaBlock *block, const SsaVar var) {
    for (size_t i = 0; i < block->outs_len; i++)
        if (block->outs[i] == var)
            return true;

    for (long int i = block->ops_len - 1; i >= 0; i--) {
        const SsaOp *op = &block->ops[i];
        for (size_t j = 0; j < op->params_len; j++) {
            if (op->params[j].val.type == SSA_VAL_BLOCK) {
                if (irblock_var_used(op->params[j].val.block, var)) {
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
bool irop_anyparam_hasvar(SsaOp *op, SsaVar var) {
    for (size_t i = 0; i < op->params_len; i ++)
        if (op->params[i].val.type == SSA_VAL_VAR && op->params[i].val.var == var)
            return true;
    return false;
}

SsaOp *irblock_inside_out_vardecl_before(const SsaBlock *block, const SsaVar var, size_t before) {
    while (before --> 0) {
        SsaOp *op = &block->ops[before];

        for (size_t i = 0; i < op->outs_len; i ++)
            if (op->outs[i].var == var)
                return op;
    }

    if (block->parent == NULL)
        return NULL;

    return irblock_inside_out_vardecl_before(block->parent, var, block->parent_index);
}

bool irop_is_pure(SsaOp *op) {
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
                        if (!irop_is_pure(&op->params[i].val.block->ops[j]))
                            return false;
        return true;

        default:
            return false;
    }
}
