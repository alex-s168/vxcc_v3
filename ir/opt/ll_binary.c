#include "../passes.h"

void vx_opt_ll_binary(vx_CU* cu, vx_IrBlock *block) {
    for (vx_IrOp* op = block->first; op; op = op->next) {
        switch (op->id) {
        case VX_IR_OP_EQ:
        case VX_IR_OP_NEQ:
        case VX_IR_OP_AND:
        case VX_IR_OP_OR:
        case VX_IR_OP_BITWISE_AND:
        case VX_IR_OP_BITIWSE_OR:

        case VX_IR_OP_ADD: // TODO: this might break programs ; add safeadd and safemul that don't have this problem
        case VX_IR_OP_MUL:
            {
                vx_IrValue *a = vx_IrOp_param(op, VX_IR_NAME_OPERAND_A);
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);
                vx_IrVar ov = op->outs[0].var;

                if (b->type == VX_IR_VAL_VAR && b->var == ov) {
                    vx_IrValue temp = *b;
                    *b = *a;
                    *a = temp;
                }
            } break;

        default:
            break;
        }
    }
}
