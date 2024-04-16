#include "ir/ir.h"
#include "ir/opt.h"
#include "ir/cir.h"

static int ir_test(void) {
    SsaBlock block;
    irblock_init(&block, NULL, 0);

    SsaOp for_op;
    irop_init(&for_op, SSA_OP_FOR, &block);
    irop_add_param_s(&for_op, SSA_NAME_LOOP_START, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 });

    SsaBlock cond;
    irblock_init(&cond, &block, block.ops_len);
    irblock_add_in(&cond, 2);
    {
        SsaOp cmp_op;
        irop_init(&cmp_op, SSA_OP_LT, &cond);
        irop_add_out(&cmp_op, 1, "bool");
        irop_add_param_s(&cmp_op, SSA_NAME_OPERAND_A, (SsaValue) { .type = SSA_VAL_VAR, .var = 2 });
        irop_add_param_s(&cmp_op, SSA_NAME_OPERAND_B, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 10 });

        irblock_add_op(&cond, &cmp_op);
    }
    irblock_add_out(&cond, 1);

    irop_add_param_s(&for_op, SSA_NAME_COND, (SsaValue) { .type = SSA_VAL_BLOCK, .block = &cond });

    SsaBlock loop;
    irblock_init(&loop, &block, block.ops_len);
    irblock_add_in(&loop, 0);
    // let's assume that this block prints the number

    irop_add_param_s(&for_op, SSA_NAME_LOOP_STRIDE, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 1 });
    irop_add_param_s(&for_op, SSA_NAME_LOOP_DO, (SsaValue) { .type = SSA_VAL_BLOCK, .block = &loop });

    irblock_add_op(&block, &for_op);

    irblock_make_root(&block, 3);

    if (ir_verify(&block) != 0)
        return 1;

    opt(&block);

    irblock_dump(&block, stdout, 0);

    return 0;
}

static int cir_test(void) {
    SsaBlock block;
    irblock_init(&block, NULL, 0);

    {
        SsaOp op;
        irop_init(&op, SSA_OP_IMM, &block);
        irop_add_out(&op, 0, "int");
        irop_add_param_s(&op, SSA_NAME_VALUE, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 1 });
        irblock_add_op(&block, &op);
    }

    {
        SsaOp op;
        irop_init(&op, SSA_OP_IF, &block);

        SsaBlock *then = irblock_heapalloc(&block, block.ops_len);
        {
            SsaOp op2;
            irop_init(&op2, SSA_OP_IMM, &block);
            irop_add_out(&op2, 0, "int");
            irop_add_param_s(&op2, SSA_NAME_VALUE, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 2 });
            irblock_add_op(then, &op2);
        }
        irop_add_param_s(&op, SSA_NAME_COND_THEN, (SsaValue) { .type = SSA_VAL_BLOCK, .block = then });

        SsaBlock *cond = irblock_heapalloc(&block, block.ops_len);
        irop_add_param_s(&op, SSA_NAME_COND, (SsaValue) { .type = SSA_VAL_BLOCK, .block = cond });

        irblock_add_op(&block, &op);
    }

    irblock_make_root(&block, 1);

    if (cir_verify(&block) != 0)
        return 1;

    cirblock_mksa_states(&block);
    cirblock_mksa_final(&block);

    irblock_destroy(&block);

    return 0;
}

int main(void) {
    printf("C-IR test:\n");
    if (cir_test() != 0)
        return 1;

    printf("\nSSA-IR test:\n");
    if (ir_test() != 0)
        return 1;

    return 0;
}
