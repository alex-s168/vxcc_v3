#include "ir/ir.h"
#include "ir/opt.h"
#include "ir/cir.h"

static int ir_test(void) {
    vx_IrBlock block;
    vx_IrBlock_init(&block, NULL, 0);

    vx_IrOp for_op;
    vx_IrOp_init(&for_op, VX_IR_OP_FOR, &block);
    vx_IrOp_add_param_s(&for_op, VX_IR_NAME_LOOP_START, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 0 });

    vx_IrBlock cond;
    vx_IrBlock_init(&cond, &block, block.ops_len);
    vx_IrBlock_add_in(&cond, 2);
    {
        vx_IrOp cmp_op;
        vx_IrOp_init(&cmp_op, VX_IR_OP_LT, &cond);
        vx_IrOp_add_out(&cmp_op, 1, "bool");
        vx_IrOp_add_param_s(&cmp_op, VX_IR_NAME_OPERAND_A, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = 2 });
        vx_IrOp_add_param_s(&cmp_op, VX_IR_NAME_OPERAND_B, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 10 });

        vx_IrBlock_add_op(&cond, &cmp_op);
    }
    vx_IrBlock_add_out(&cond, 1);

    vx_IrOp_add_param_s(&for_op, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = &cond });

    vx_IrBlock loop;
    vx_IrBlock_init(&loop, &block, block.ops_len);
    vx_IrBlock_add_in(&loop, 0);
    // let's assume that this block prints the number

    vx_IrOp_add_param_s(&for_op, VX_IR_NAME_LOOP_STRIDE, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 1 });
    vx_IrOp_add_param_s(&for_op, VX_IR_NAME_LOOP_DO, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = &loop });

    vx_IrBlock_add_op(&block, &for_op);

    vx_IrBlock_make_root(&block, 3);

    if (vx_ir_verify(&block) != 0)
        return 1;

    opt(&block);

    vx_IrBlock_dump(&block, stdout, 0);

    return 0;
}

static vx_IrBlock *always_true_block(vx_IrBlock *parent, vx_IrVar temp_var) {
    vx_IrBlock *block = vx_IrBlock_init_heap(parent, parent->ops_len);

    vx_IrOp assign;
    vx_IrOp_init(&assign, VX_IR_OP_IMM, block);
    vx_IrOp_add_out(&assign, temp_var, "int");
    vx_IrOp_add_param_s(&assign, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 1 });
    vx_IrBlock_add_op(block, &assign);

    vx_IrBlock_add_out(block, temp_var);
    return block;
}

static vx_IrBlock *conditional_c_assign(vx_IrVar dest, vx_IrBlock *parent, vx_IrVar temp_var) {
    vx_IrOp op;
    vx_IrOp_init(&op, VX_IR_OP_IF, parent);
    
    vx_IrBlock *then = vx_IrBlock_init_heap(parent, parent->ops_len);
    {
        vx_IrOp op2;
        vx_IrOp_init(&op2, VX_IR_OP_IMM, then);
        vx_IrOp_add_out(&op2, dest, "int");
        vx_IrOp_add_param_s(&op2, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 2 });
        vx_IrBlock_add_op(then, &op2);
    }
    vx_IrOp_add_param_s(&op, VX_IR_NAME_COND_THEN, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = then });

    vx_IrOp_add_param_s(&op, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = always_true_block(parent, temp_var) });

    vx_IrBlock_add_op(parent, &op);

    return then;
}

static vx_IrBlock *conditional_c_assign_else(vx_IrVar dest, vx_IrBlock *parent, vx_IrVar temp_var) {
    vx_IrOp op;
    vx_IrOp_init(&op, VX_IR_OP_IF, parent);
    
    vx_IrOp_add_param_s(&op, VX_IR_NAME_COND_THEN, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = vx_IrBlock_init_heap(parent, parent->ops_len) });

    vx_IrBlock *els = vx_IrBlock_init_heap(parent, parent->ops_len);
    {
        vx_IrOp op2;
        vx_IrOp_init(&op2, VX_IR_OP_IMM, els);
        vx_IrOp_add_out(&op2, dest, "int");
        vx_IrOp_add_param_s(&op2, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 2 });
        vx_IrBlock_add_op(els, &op2);
    }
    vx_IrOp_add_param_s(&op, VX_IR_NAME_COND_ELSE, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = els });

    vx_IrOp_add_param_s(&op, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = always_true_block(parent, temp_var) });

    vx_IrBlock_add_op(parent, &op);

    return els;
}

static int cir_test(void) {
    vx_IrBlock block;
    vx_IrBlock_init(&block, NULL, 0);

    {
        vx_IrOp op;
        vx_IrOp_init(&op, VX_IR_OP_IMM, &block);
        vx_IrOp_add_out(&op, 0, "int");
        vx_IrOp_add_param_s(&op, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 1 });
        vx_IrBlock_add_op(&block, &op);
    }

    vx_IrBlock *els = conditional_c_assign_else(0, &block, 1);
    conditional_c_assign_else(0, els, 2);

    {
        vx_IrOp op;
        vx_IrOp_init(&op, VX_IR_OP_IMM, &block);
        vx_IrOp_add_out(&op, 1, "int");
        vx_IrOp_add_param_s(&op, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = 0 });
        vx_IrBlock_add_op(&block, &op);
    }

    vx_IrBlock_make_root(&block, 1);

    if (vx_cir_verify(&block) != 0)
        return 1;

    printf("Pre:\n");
    vx_IrBlock_dump(&block, stdout, 0);

    vx_CIrBlock_mksa_states(&block);
    vx_CIrBlock_mksa_final(&block);

    printf("Post:\n");
    vx_IrBlock_dump(&block, stdout, 0);

    vx_IrBlock_destroy(&block);

    return 0;
}

int main(void) {
    printf("C-IR test:\n");
    if (cir_test() != 0)
        return 1;

    //printf("\nSSA-IR test:\n");
    //if (ir_test() != 0)
    //    return 1;

    return 0;
}
