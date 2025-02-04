#include <assert.h>
#include "../passes.h"

void vx_opt_constant_eval(vx_CU* cu, vx_IrBlock *block)
{
	// TODO: only if arg type sizes < sizeof(long long)
    for (vx_IrOp *op = block->first; op; op = op->next) {
#define BINARY(typedest, what) { \
        vx_IrValue *a = vx_IrOp_param(op, VX_IR_NAME_OPERAND_A); \
        vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B); \
        vx_Irblock_eval(block, a);      \
        vx_Irblock_eval(block, b);      \
                                            \
        if (a->type == VX_IR_VAL_IMM_INT && b->type == VX_IR_VAL_IMM_INT) { \
            op->id = VX_IR_OP_IMM; \
            vx_IrOp_removeParams(op); \
            vx_IrOp_addParam_s(op, VX_IR_NAME_VALUE, (vx_IrValue) { \
                .type = VX_IR_VAL_IMM_INT, \
                .imm_int = ((typedest) a->imm_int) what ((typedest) b->imm_int) \
            }); \
        } else if (a->type == VX_IR_VAL_IMM_FLT && b->type == VX_IR_VAL_IMM_FLT) { \
            op->id = VX_IR_OP_IMM; \
            vx_IrOp_removeParams(op); \
            vx_IrOp_addParam_s(op, VX_IR_NAME_VALUE, (vx_IrValue) { \
                .type = VX_IR_VAL_IMM_FLT, \
                .imm_flt = ((typedest) a->imm_flt) what ((typedest) b->imm_flt) \
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
                BINARY(signed long long, +);
                break;

            case VX_IR_OP_SUB:
                BINARY(signed long long, -);
                break;

            case VX_IR_OP_MUL:
                BINARY(signed long long, *);
                break;

            case VX_IR_OP_SDIV:
                BINARY(signed long long, /);
                break;

            case VX_IR_OP_UDIV:
                BINARY(unsigned long long, /);
                break;

            case VX_IR_OP_UMOD:
                BINARY(unsigned long long, %);
                break;

            case VX_IR_OP_SMOD:
                BINARY(signed long long, %);
                break;

            case VX_IR_OP_AND:
                BINARY(unsigned long long, &&);
                break;

            case VX_IR_OP_OR:
                BINARY(unsigned long long, ||);
                break;

            case VX_IR_OP_BITWISE_AND:
                BINARY(unsigned long long, &);
                break;

            case VX_IR_OP_BITIWSE_OR:
                BINARY(unsigned long long, |);
                break;

            case VX_IR_OP_SHL:
                BINARY(unsigned long long, <<);
                break;

            case VX_IR_OP_SHR:
                BINARY(unsigned long long, >>);
                break;

            case VX_IR_OP_ASHR:
                BINARY(signed long long, >>);
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
                BINARY(signed long long, ==);
                break;

            case VX_IR_OP_NEQ:
                BINARY(signed long long, !=);
                break;

            case VX_IR_OP_SLT:
                BINARY(signed long long, <);
                break;

            case VX_IR_OP_SLTE:
                BINARY(signed long long, <=);
                break;

            case VX_IR_OP_SGT:
                BINARY(signed long long, >);
                break;

            case VX_IR_OP_SGTE:
                BINARY(signed long long, >=);
                break;

            case VX_IR_OP_ULT:
                BINARY(unsigned long long, <);
                break;

            case VX_IR_OP_ULTE:
                BINARY(unsigned long long, <=);
                break;

            case VX_IR_OP_UGT:
                BINARY(unsigned long long, >);
                break;

            case VX_IR_OP_UGTE:
                BINARY(unsigned long long, >=);
                break;

            default:
                break;
        }
    }
}
