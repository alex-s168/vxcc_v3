#include "../opt.h"

/**
 * `a <= constant` -> `a < constant + 1`
 * `a >= constant` -> `a > constant - 1`
 * `constant < a` -> `a > constant`
 * `constant > a` -> `a < constant`
 */
void vx_opt_comparisions(vx_IrView view,
                         vx_IrBlock *block)
{
    for (size_t i = view.start; i < view.end; i ++) {
        vx_IrOp *op = &block->ops[i];

        switch (op->id) {
            case VX_IR_OP_LTE: {
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (b->type == VX_IR_VAL_IMM_INT) {
                    b->imm_int ++;
                    op->id = VX_IR_OP_LT;
                }
            } break;

            case VX_IR_OP_GTE: {
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (b->type == VX_IR_VAL_IMM_INT && b->imm_int != 0) {
                    b->imm_int --;
                    op->id = VX_IR_OP_GT;
                }
            } break;

            case VX_IR_OP_LT: {
                vx_IrValue *a = vx_IrOp_param(op, VX_IR_NAME_OPERAND_A);
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (a->type == VX_IR_VAL_IMM_INT) {
                    const vx_IrValue tmp = *a;
                    *a = *b;
                    *b = tmp;
                    op->id = VX_IR_OP_GT;
                }
            } break;

            case VX_IR_OP_GT: {
                vx_IrValue *a = vx_IrOp_param(op, VX_IR_NAME_OPERAND_A);
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                if (a->type == VX_IR_VAL_IMM_INT) {
                    const vx_IrValue tmp = *a;
                    *a = *b;
                    *b = tmp;
                    op->id = VX_IR_OP_LT;
                }
            } break;

            default: break;
        }
    }
}
