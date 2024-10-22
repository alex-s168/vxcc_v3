#include "../opt.h"

/**
 * `a <= constant` -> `a < constant + 1`
 * `a >= constant` -> `a > constant - 1`
 * `constant < a` -> `a > constant`
 * `constant > a` -> `a < constant`
 * `!cmp(a,b)` -> `ncmp(a,b)`
 * `constant == a` -> `a == constant`
 */
void vx_opt_comparisions(vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        switch (op->id) {
            case VX_IR_OP_NOT: {
                vx_IrValue* oldres = vx_IrOp_param(op, VX_IR_NAME_VALUE);
                vx_IrOp* cmp;
                if (oldres->type == VX_IR_VAL_VAR &&
                        (cmp = vx_IrBlock_vardeclOutBefore(block, oldres->var, op)))
                {
                    vx_IrValue* a = vx_IrOp_param(cmp, VX_IR_NAME_OPERAND_A);
                    vx_IrValue* b = vx_IrOp_param(cmp, VX_IR_NAME_OPERAND_B);

                    vx_IrOpType__entry* e = &vx_IrOpType__entries[cmp->id];
                    if (a && b && e->boolInv.set) {
                        // TODO: make this id instead (for speed)
                        bool found = vx_IrOpType_parsec(&op->id, e->boolInv.a);
                        assert(found && "inverted op id not found");
                        vx_IrOp_removeParams(op);
                        vx_IrOp_addParam_s(op, VX_IR_NAME_OPERAND_A, *a);
                        vx_IrOp_addParam_s(op, VX_IR_NAME_OPERAND_B, *b);
                    }
                }
            } break;

            case VX_IR_OP_EQ:
            case VX_IR_OP_NEQ: {
                vx_IrValue *a = vx_IrOp_param(op, VX_IR_NAME_OPERAND_A);
                vx_IrValue *b = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);

                if (a->type == VX_IR_VAL_IMM_INT)
                {
                    vx_IrValue tmp = *a;
                    *a = *b;
                    *b = tmp;
                }
            } break;

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
