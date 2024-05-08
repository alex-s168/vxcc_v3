#include "../opt.h"

#include <assert.h>

void vx_opt_constant_eval(vx_IrView view,
                          vx_IrBlock *block)
{
    assert(view.block == block);

    while (vx_IrView_has_next(view)) {
        // "unsafe", but we own the underlying block anyways
        vx_IrOp *op = (vx_IrOp *) vx_IrView_take(view);

#define BINARY(what) { \
        vx_IrValue *a = vx_IrOp_param(op, VX_IR_NAME_OPERAND_A); \
        vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B); \
        vx_Irblock_eval(block, a);      \
        vx_Irblock_eval(block, b);      \
                                            \
        if (a->type == VX_IR_VAL_IMM_INT && b->type == VX_IR_VAL_IMM_INT) { \
            op->id = VX_IR_OP_IMM; \
            vx_IrOp_remove_params(op); \
            vx_IrOp_add_param_s(op, VX_IR_NAME_VALUE, (vx_IrValue) { \
                .type = VX_IR_VAL_IMM_INT, \
                .imm_int = a->imm_int what b->imm_int \
            }); \
        } else if (a->type == VX_IR_VAL_IMM_FLT && b->type == VX_IR_VAL_IMM_FLT) { \
            op->id = VX_IR_OP_IMM; \
            vx_IrOp_remove_params(op); \
            vx_IrOp_add_param_s(op, VX_IR_NAME_VALUE, (vx_IrValue) { \
                .type = VX_IR_VAL_IMM_FLT, \
                .imm_flt = a->imm_flt + b->imm_flt \
            }); \
        } \
        }

#define UNARY(what) { \
        vx_IrValue *val = vx_IrOp_param(op, VX_IR_NAME_VALUE); \
        vx_Irblock_eval(block, val); \
        if (val->type == VX_IR_VAL_IMM_INT) { \
            op->id = VX_IR_OP_IMM; \
            *val = (vx_IrValue) { \
                .type = VX_IR_VAL_IMM_INT, \
                .imm_int = what val->imm_int \
            }; \
        } else if (val->type == VX_IR_VAL_IMM_FLT) { \
            op->id = VX_IR_OP_IMM; \
            *val = (vx_IrValue) { \
                .type = VX_IR_VAL_IMM_FLT, \
                .imm_flt = what val->imm_flt \
            }; \
        } \
        }
        
        switch (op->id) {
            case VX_IR_OP_ADD:
                BINARY(+);
                break;

            case VX_IR_OP_SUB:
                BINARY(-);
                break;

            case VX_IR_OP_MUL:
                BINARY(*);
                break;

            case VX_IR_OP_DIV:
                BINARY(/);
                break;

            case VX_IR_OP_MOD:
                BINARY(%);
                break;

            case VX_IR_OP_AND:
                BINARY(&&);
                break;

            case VX_IR_OP_OR:
                BINARY(||);
                break;

            case VX_IR_OP_BITWISE_AND:
                BINARY(&);
                break;

            case VX_IR_OP_BITIWSE_OR:
                BINARY(|);
                break;

            case VX_IR_OP_SHL:
                BINARY(<<);
                break;

            case VX_IR_OP_SHR:
                BINARY(>>);
                break;

            case VX_IR_OP_NOT:
                UNARY(!);
                break;

            case VX_IR_OP_BITWISE_NOT: {
                vx_IrValue *val = vx_IrOp_param(op, VX_IR_NAME_VALUE);
                vx_Irblock_eval(block, val);
                if (val->type == VX_IR_VAL_IMM_INT) {
                    op->id = VX_IR_OP_IMM;
                    *val = (vx_IrValue) {
                        .type = VX_IR_VAL_IMM_INT,
                        .imm_int = ~val->imm_int
                    };
                }
            } break;

            case VX_IR_OP_EQ:
                BINARY(==);
                break;

            case VX_IR_OP_NEQ:
                BINARY(!=);
                break;

            case VX_IR_OP_LT:
                BINARY(<);
                break;

            case VX_IR_OP_LTE:
                BINARY(<=);
                break;

            case VX_IR_OP_GT:
                BINARY(>);
                break;

            case VX_IR_OP_GTE:
                BINARY(>=);
                break;

            default:
                break;
        }

        view = vx_IrView_drop(view, 1);
    }
}
