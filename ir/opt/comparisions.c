#include "../opt.h"

/**
 * `a <= constant` -> `a < constant + 1`
 * `a >= constant` -> `a > constant - 1`
 * `constant < a` -> `a > constant`
 * `constant > a` -> `a < constant`
 */
void vx_opt_comparisions(vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        switch (op->id) {
            case VX_IR_OP_ULTE: {
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (b->type == VX_IR_VAL_IMM_INT) {
                    b->imm_int ++;
                    op->id = VX_IR_OP_ULT;
                }
            } break;

            case VX_IR_OP_UGTE: {
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (b->type == VX_IR_VAL_IMM_INT && b->imm_int != 0) {
                    b->imm_int --;
                    op->id = VX_IR_OP_UGT;
                }
            } break;

            case VX_IR_OP_ULT: {
                vx_IrValue *a = vx_IrOp_param(op, VX_IR_NAME_OPERAND_A);
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (a->type == VX_IR_VAL_IMM_INT) {
                    const vx_IrValue tmp = *a;
                    *a = *b;
                    *b = tmp;
                    op->id = VX_IR_OP_UGT;
                }
            } break;

            case VX_IR_OP_UGT: {
                vx_IrValue *a = vx_IrOp_param(op, VX_IR_NAME_OPERAND_A);
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (a->type == VX_IR_VAL_IMM_INT) {
                    const vx_IrValue tmp = *a;
                    *a = *b;
                    *b = tmp;
                    op->id = VX_IR_OP_ULT;
                }
            } break;

            // signed

            case VX_IR_OP_SLTE: {
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (b->type == VX_IR_VAL_IMM_INT) {
                    b->imm_int ++;
                    op->id = VX_IR_OP_SLT;
                }
            } break;

            case VX_IR_OP_SGTE: {
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (b->type == VX_IR_VAL_IMM_INT && b->imm_int != 0) {
                    b->imm_int --;
                    op->id = VX_IR_OP_SGT;
                }
            } break;

            case VX_IR_OP_SLT: {
                vx_IrValue *a = vx_IrOp_param(op, VX_IR_NAME_OPERAND_A);
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (a->type == VX_IR_VAL_IMM_INT) {
                    const vx_IrValue tmp = *a;
                    *a = *b;
                    *b = tmp;
                    op->id = VX_IR_OP_SGT;
                }
            } break;

            case VX_IR_OP_SGT: {
                vx_IrValue *a = vx_IrOp_param(op, VX_IR_NAME_OPERAND_A);
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (a->type == VX_IR_VAL_IMM_INT) {
                    const vx_IrValue tmp = *a;
                    *a = *b;
                    *b = tmp;
                    op->id = VX_IR_OP_SLT;
                }
            } break;

            default: break;
        }
    }
}
