#include <stdio.h>

#include "opt.h"
#include "ssa.h"

int main(void) {
    SsaBlock block;
    ssablock_init(&block, NULL);

    SsaOp for_op;
    ssaop_init(&for_op, SSA_OP_FOR);
    ssaop_add_param_s(&for_op, "init", (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 0 });
    
    SsaBlock cond;
    ssablock_init(&cond, &block);
    ssablock_add_in(&cond, 0);
    {
        SsaOp cmp_op;
        ssaop_init(&cmp_op, SSA_OP_LT);
        ssaop_add_out(&cmp_op, 1, "bool");
        ssaop_add_param_s(&cmp_op, "a", (SsaValue) { .type = SSA_VAL_VAR, .var = 0 });
        ssaop_add_param_s(&cmp_op, "b", (SsaValue) { .type = SSA_VAL_IMM_INT, .imm_int = 10 });
        
        ssablock_add_op(&cond, &cmp_op);
    }
    ssablock_add_out(&cond, 1);
    
    ssaop_add_param_s(&for_op, "cond", (SsaValue) { .type = SSA_VAL_BLOCK, .block = &cond });

    SsaBlock loop;
    ssablock_init(&loop, &block);
    ssablock_add_in(&loop, 0);
    // let's assume that this block prints the number

    ssaop_add_param_s(&for_op, "do", (SsaValue) { .type = SSA_VAL_BLOCK, .block = &loop });

    ssablock_add_op(&block, &for_op);

    opt(&block);
    
    return 0;
}
