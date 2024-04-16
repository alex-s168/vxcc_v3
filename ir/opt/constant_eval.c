#include "../opt.h"

#include <assert.h>

void opt_constant_eval(SsaView view, SsaBlock *block) {
    assert(view.block == block);

    while (irview_has_next(view)) {
        // "unsafe", but we own the underlying block anyways
        SsaOp *op = (SsaOp *) irview_take(view);

#define BINARY(what) { \
    SsaValue *a = irop_param(op, SSA_NAME_OPERAND_A); \
    SsaValue *b = irop_param(op, SSA_NAME_OPERAND_B); \
    irblock_staticeval(block, a);      \
    irblock_staticeval(block, b);      \
                                        \
    if (a->type == SSA_VAL_IMM_INT && b->type == SSA_VAL_IMM_INT) { \
        op->id = SSA_OP_IMM; \
        irop_remove_params(op); \
        irop_add_param_s(op, SSA_NAME_VALUE, (SsaValue) { \
            .type = SSA_VAL_IMM_INT, \
            .imm_int = a->imm_int what b->imm_int \
        }); \
    } else if (a->type == SSA_VAL_IMM_FLT && b->type == SSA_VAL_IMM_FLT) { \
        op->id = SSA_OP_IMM; \
        irop_remove_params(op); \
        irop_add_param_s(op, SSA_NAME_VALUE, (SsaValue) { \
            .type = SSA_VAL_IMM_FLT, \
            .imm_flt = a->imm_flt + b->imm_flt \
        }); \
    } \
}

#define UNARY(what) { \
    SsaValue *val = irop_param(op, SSA_NAME_VALUE); \
    irblock_staticeval(block, val); \
    if (val->type == SSA_VAL_IMM_INT) { \
        op->id = SSA_OP_IMM; \
        *val = (SsaValue) { \
            .type = SSA_VAL_IMM_INT, \
            .imm_int = what val->imm_int \
        }; \
    } else if (val->type == SSA_VAL_IMM_FLT) { \
        op->id = SSA_OP_IMM; \
        *val = (SsaValue) { \
            .type = SSA_VAL_IMM_FLT, \
            .imm_flt = what val->imm_flt \
        }; \
    } \
}

        switch (op->id) {
            case SSA_OP_ADD:
                BINARY(+);
                break;

            case SSA_OP_SUB:
                BINARY(-);
                break;

            case SSA_OP_MUL:
                BINARY(*);
                break;

            case SSA_OP_DIV:
                BINARY(/);
                break;

            case SSA_OP_MOD:
                BINARY(%);
                break;

            case SSA_OP_AND:
                BINARY(&&);
                break;

            case SSA_OP_OR:
                BINARY(||);
                break;

            case SSA_OP_BITWISE_AND:
                BINARY(&);
                break;

            case SSA_OP_BITIWSE_OR:
                BINARY(|);
                break;

            case SSA_OP_SHL:
                BINARY(<<);
                break;

            case SSA_OP_SHR:
                BINARY(>>);
                break;

            case SSA_OP_NOT:
                UNARY(!);
                break;

            case SSA_OP_BITWISE_NOT: {
                SsaValue *val = irop_param(op, SSA_NAME_VALUE);
                irblock_staticeval(block, val);
                if (val->type == SSA_VAL_IMM_INT) {
                    op->id = SSA_OP_IMM;
                    *val = (SsaValue) {
                        .type = SSA_VAL_IMM_INT,
                        .imm_int = ~val->imm_int
                    };
                }
            } break;

            case SSA_OP_EQ:
                BINARY(==);
                break;

            case SSA_OP_NEQ:
                BINARY(!=);
                break;

            case SSA_OP_LT:
                BINARY(<);
                break;

            case SSA_OP_LTE:
                BINARY(<=);
                break;

            case SSA_OP_GT:
                BINARY(>);
                break;

            case SSA_OP_GTE:
                BINARY(>=);
                break;

            default:
                break;
        }

        view = irview_drop(view, 1);
    }
}
