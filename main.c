#include "ir/ir.h"
#include "ir/cir.h"
#include "ir/llir.h"
#include "ir/opt.h"
#include "cg/x86/x86.h"

static vx_IrType *ty_int;

static vx_IrBlock *always_true_block(vx_IrBlock *parent, vx_IrVar temp_var) {
    vx_IrBlock *block = vx_IrBlock_init_heap(parent, parent->ops_len);

    vx_IrOp assign;
    vx_IrOp_init(&assign, VX_IR_OP_IMM, block);
    vx_IrOp_add_out(&assign, temp_var, ty_int);
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
        vx_IrOp_add_out(&op2, dest, ty_int);
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
        vx_IrOp_add_out(&op2, dest, ty_int);
        vx_IrOp_add_param_s(&op2, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 2 });
        vx_IrBlock_add_op(els, &op2);
    }
    vx_IrOp_add_param_s(&op, VX_IR_NAME_COND_ELSE, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = els });

    vx_IrOp_add_param_s(&op, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = always_true_block(parent, temp_var) });

    vx_IrBlock_add_op(parent, &op);

    return els;
}

static int cir_test(void) {
    vx_IrBlock *block = vx_IrBlock_init_heap(NULL, 0);

    {
        vx_IrOp op;
        vx_IrOp_init(&op, VX_IR_OP_IMM, block);
        vx_IrOp_add_out(&op, 0, ty_int);
        vx_IrOp_add_param_s(&op, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 1 });
        vx_IrBlock_add_op(block, &op);
    }

    vx_IrBlock *els = conditional_c_assign_else(0, block, 1);
    conditional_c_assign_else(0, els, 2);

    {
        vx_IrOp op;
        vx_IrOp_init(&op, VX_IR_OP_IMM, block);
        vx_IrOp_add_out(&op, 1, ty_int);
        vx_IrOp_add_param_s(&op, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = 0 });
        vx_IrBlock_add_op(block, &op);
    }

    vx_IrBlock_make_root(block, 1);

    if (vx_cir_verify(block) != 0)
        return 1;

    printf("Input:\n");
    vx_IrBlock_dump(block, stdout, 0);

    vx_CIrBlock_mksa_states(block);
    vx_CIrBlock_mksa_final(block);

    printf("After C IR lower:\n");
    vx_IrBlock_dump(block, stdout, 0);

    if (vx_ir_verify(block) != 0)
        return 1;

    opt(block);

    printf("After SSA IR opt:\n");
    vx_IrBlock_dump(block, stdout, 0);

    if (vx_ir_verify(block) != 0)
        return 1;

    vx_IrBlock_llir_lower(block);
    vx_x86cg_prepare(block);

    printf("After SSA IR lower & cg prepare:\n");
    vx_IrBlock_dump(block, stdout, 0);

    opt_ll(block);

    printf("After LL IR opt:\n");
    vx_IrBlock_dump(block, stdout, 0);

    llir_prep_lower(block);

    printf("After LL IR lower prepare:\n");
    vx_IrBlock_dump(block, stdout, 0);

    vx_IrBlock_destroy(block);

    return 0;
}

int main(void) {
    // TODO: figure out why these break things
    vx_g_optconfig.loop_simplify = false;
    vx_g_optconfig.if_eval = false;

    ty_int = vx_IrType_heap();
    ty_int->debugName = "i32";
    ty_int->kind = VX_IR_TYPE_KIND_BASE;
    ty_int->base.align = 4;
    ty_int->base.size = 4;

    printf("C-IR test:\n");
    if (cir_test() != 0)
        return 1;

    //printf("\nSSA-IR test:\n");
    //if (ir_test() != 0)
    //    return 1;

    return 0;
}
