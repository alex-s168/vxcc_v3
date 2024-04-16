#include "../opt.h"

/**
 * `a <= constant` -> `a < constant + 1`
 * `a >= constant` -> `a > constant - 1`
 * `constant < a` -> `a > constant`
 * `constant > a` -> `a < constant`
 */
void opt_comparisions(SsaView view, SsaBlock *block) {
    for (size_t i = view.start; i < view.end; i ++) {
        SsaOp *op = &block->ops[i];

        switch (op->id) {
            case SSA_OP_LTE: {
                SsaValue *b = ssaop_param(op, SSA_NAME_OPERAND_B);
                if (b->type == SSA_VAL_IMM_INT) {
                    b->imm_int ++;
                    op->id = SSA_OP_LT;
                }
            } break;

            case SSA_OP_GTE: {
                SsaValue *b = ssaop_param(op, SSA_NAME_OPERAND_B);
                if (b->type == SSA_VAL_IMM_INT) {
                    b->imm_int --;
                    op->id = SSA_OP_GT;
                }
            } break;

            case SSA_OP_LT: {
                SsaValue *a = ssaop_param(op, SSA_NAME_OPERAND_A);
                SsaValue *b = ssaop_param(op, SSA_NAME_OPERAND_B);
                if (a->type == SSA_VAL_IMM_INT) {
                    const SsaValue tmp = *a;
                    *a = *b;
                    *b = tmp;
                    op->id = SSA_OP_GT;
                }
            } break;

            case SSA_OP_GT: {
                SsaValue *a = ssaop_param(op, SSA_NAME_OPERAND_A);
                SsaValue *b = ssaop_param(op, SSA_NAME_OPERAND_B);
                if (a->type == SSA_VAL_IMM_INT) {
                    const SsaValue tmp = *a;
                    *a = *b;
                    *b = tmp;
                    op->id = SSA_OP_LT;
                }
            } break;

            default: break;
        }
    }
}
