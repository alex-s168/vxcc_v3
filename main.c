#include "ir_ssa/ssa.h"
#include "ir_ssa/opt.h"
#include "ir_ssa/cir.h"

static int ssa_test(void) {
    SsaBlock block;
    ssablock_init(&block, NULL, 0);

    SsaOp for_op;
    ssaop_init(&for_op, SSA_OP_FOR, &block);
    ssaop_add_param_s(&for_op, SSA_NAME_LOOP_START, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 });

    SsaBlock cond;
    ssablock_init(&cond, &block, block.ops_len);
    ssablock_add_in(&cond, 2);
    {
        SsaOp cmp_op;
        ssaop_init(&cmp_op, SSA_OP_LT, &cond);
        ssaop_add_out(&cmp_op, 1, "bool");
        ssaop_add_param_s(&cmp_op, SSA_NAME_OPERAND_A, (SsaValue) { .type = SSA_VAL_VAR, .var = 2 });
        ssaop_add_param_s(&cmp_op, SSA_NAME_OPERAND_B, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 10 });

        ssablock_add_op(&cond, &cmp_op);
    }
    ssablock_add_out(&cond, 1);

    ssaop_add_param_s(&for_op, SSA_NAME_COND, (SsaValue) { .type = SSA_VAL_BLOCK, .block = &cond });

    SsaBlock loop;
    ssablock_init(&loop, &block, block.ops_len);
    ssablock_add_in(&loop, 0);
    // let's assume that this block prints the number

    ssaop_add_param_s(&for_op, SSA_NAME_LOOP_STRIDE, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 1 });
    ssaop_add_param_s(&for_op, SSA_NAME_LOOP_DO, (SsaValue) { .type = SSA_VAL_BLOCK, .block = &loop });

    ssablock_add_op(&block, &for_op);

    ssablock_make_root(&block, 3);

    if (ssa_verify(&block) != 0)
        return 1;

    opt(&block);

    ssablock_dump(&block, stdout, 0);

    return 0;
}

static int cir_test(void) {
    SsaBlock block;
    ssablock_init(&block, NULL, 0);

    {
        SsaOp op;
        ssaop_init(&op, SSA_OP_IMM, &block);
        ssaop_add_out(&op, 0, "int");
        ssaop_add_param_s(&op, SSA_NAME_VALUE, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 1 });
        ssablock_add_op(&block, &op);
    }

    {
        SsaOp op;
        ssaop_init(&op, SSA_OP_IF, &block);

        SsaBlock *then = ssablock_heapalloc(&block, block.ops_len);
        {
            SsaOp op2;
            ssaop_init(&op2, SSA_OP_IMM, &block);
            ssaop_add_out(&op2, 0, "int");
            ssaop_add_param_s(&op2, SSA_NAME_VALUE, (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 2 });
            ssablock_add_op(then, &op2);
        }
        ssaop_add_param_s(&op, SSA_NAME_COND_THEN, (SsaValue) { .type = SSA_VAL_BLOCK, .block = then });

        SsaBlock *cond = ssablock_heapalloc(&block, block.ops_len);
        ssaop_add_param_s(&op, SSA_NAME_COND, (SsaValue) { .type = SSA_VAL_BLOCK, .block = cond });

        ssablock_add_op(&block, &op);
    }

    ssablock_make_root(&block, 1);

    if (cir_verify(&block) != 0)
        return 1;

    cirblock_mksa_states(&block);
    cirblock_mksa_final(&block);

    ssablock_destroy(&block);

    return 0;
}

int main(void) {
    printf("C-IR test:\n");
    if (cir_test() != 0)
        return 1;

    printf("\nSSA-IR test:\n");
    if (ssa_test() != 0)
        return 1;

    return 0;
}
