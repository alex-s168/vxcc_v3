#include <stdio.h>
#include "ir/ir.h"
#include "ir/cir.h"
#include "ir/llir.h"
#include "ir/opt.h"
#include "cg/x86_stupid/cg.h"

static vx_IrType *ty_int;
static vx_IrType *ty_bool;

static vx_IrBlock *var_block(vx_IrBlock *parent, vx_IrVar var) {
    vx_IrBlock *block = vx_IrBlock_init_heap(parent, parent->parent_op);
    vx_IrBlock_add_out(block, var);
    return block;
}

static vx_IrBlock *always_true_block(vx_IrBlock *parent, vx_IrVar temp_var) {
    vx_IrBlock *block = vx_IrBlock_init_heap(parent, parent->parent_op);

    vx_IrOp *assign = vx_IrBlock_add_op_building(block);
    vx_IrOp_init(assign, VX_IR_OP_IMM, block);
    vx_IrOp_add_out(assign, temp_var, ty_int);
    vx_IrOp_add_param_s(assign, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 1 });

    vx_IrBlock_add_out(block, temp_var);
    return block;
}

static vx_IrBlock *conditional_c_assign_else(vx_IrVar dest, vx_IrBlock *parent, long long value, vx_IrVar condVar) {
    vx_IrOp *op = vx_IrBlock_add_op_building(parent);
    vx_IrOp_init(op, VX_IR_OP_IF, parent);
    
    vx_IrOp_add_param_s(op, VX_IR_NAME_COND_THEN, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = vx_IrBlock_init_heap(parent, parent->parent_op) });

    vx_IrBlock *els = vx_IrBlock_init_heap(parent, parent->parent_op);
    {
        vx_IrOp *op2 = vx_IrBlock_add_op_building(els);
        vx_IrOp_init(op2, VX_IR_OP_IMM, els);
        vx_IrOp_add_out(op2, dest, ty_int);
        vx_IrOp_add_param_s(op2, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = value });
    }
    vx_IrOp_add_param_s(op, VX_IR_NAME_COND_ELSE, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = els });

    vx_IrOp_add_param_s(op, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = var_block(parent, condVar) });

    return els;
}

static void gen_call_op(vx_IrBlock *dest, vx_IrBlock* block) {
    vx_IrOp *op = vx_IrBlock_add_op_building(dest);
    vx_IrOp_init(op, VX_IR_OP_CALL, dest);

    vx_IrOp_add_param_s(op, VX_IR_NAME_ADDR, (vx_IrValue) {.type = VX_IR_VAL_BLOCKREF,.block = block});
}

static void gen_bin_op(vx_IrBlock *dest, vx_IrOpType optype, vx_IrType *outtype, vx_IrVar d, vx_IrVar a, vx_IrVar b) {
    vx_IrOp *op = vx_IrBlock_add_op_building(dest);
    vx_IrOp_init(op, optype, dest);

    vx_IrOp_add_param_s(op, VX_IR_NAME_OPERAND_A, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = a});
    vx_IrOp_add_param_s(op, VX_IR_NAME_OPERAND_B, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = b});

    vx_IrOp_add_out(op, d, outtype);
}

static vx_IrBlock * build_test_cmov(void) {
    vx_IrBlock *block = vx_IrBlock_init_heap(NULL, 0);
    vx_IrBlock_add_in(block, 10, ty_bool);
    vx_IrBlock_add_in(block, 11, ty_bool);

    {
        vx_IrOp *op = vx_IrBlock_add_op_building(block);
        vx_IrOp_init(op, VX_IR_OP_IMM, block);
        vx_IrOp_add_out(op, 0, ty_int);
        vx_IrOp_add_param_s(op, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = 1 });
    }

    vx_IrBlock *els = conditional_c_assign_else(0, block, 2, 10);
    conditional_c_assign_else(0, els, 3, 11);

    {
        vx_IrOp *op = vx_IrBlock_add_op_building(block);
        vx_IrOp_init(op, VX_IR_OP_IMM, block);
        vx_IrOp_add_out(op, 1, ty_int);
        vx_IrOp_add_param_s(op, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = 0 });
    }

    vx_IrBlock_add_out(block, 1); 
    vx_IrBlock_make_root(block, 12);

    return block;
}

static vx_IrBlock * build_test_bool(void) {
/*
void call();
void call2();

void eq(int a, int b, int c, int d) {
    if (a == b && c == d) {
        call();
    } else {
        call2();
    }
}
*/
    vx_IrBlock *call = vx_IrBlock_init_heap(NULL, 0);
    call->name = "call";

    vx_IrBlock *call2 = vx_IrBlock_init_heap(NULL, 0);
    call2->name = "call2";

    vx_IrBlock *block = vx_IrBlock_init_heap(NULL, 0);
    block->name = "eq";

    vx_IrVar a = 0, b = 1, c = 2, d = 3, temp0 = 4, temp1 = 5, temp2 = 6;

    vx_IrBlock_add_in(block, a, ty_int);
    vx_IrBlock_add_in(block, b, ty_int);
    vx_IrBlock_add_in(block, c, ty_int);
    vx_IrBlock_add_in(block, d, ty_int);

    vx_IrOp *iff = vx_IrBlock_add_op_building(block);
    vx_IrOp_init(iff, VX_IR_OP_IF, block);

    vx_IrBlock *cond = vx_IrBlock_init_heap(block, 0);
    vx_IrBlock *then = vx_IrBlock_init_heap(block, 0);
    vx_IrBlock *els  = vx_IrBlock_init_heap(block, 0);

    {
        gen_bin_op(cond, VX_IR_OP_EQ, ty_bool, temp0, a, b);
        gen_bin_op(cond, VX_IR_OP_EQ, ty_bool, temp1, c, d);
        gen_bin_op(cond, VX_IR_OP_AND, ty_bool, temp2, temp0, temp1);
        vx_IrBlock_add_out(cond, temp2);
    }

    gen_call_op(then, call);
    gen_call_op(els, call2);

    vx_IrOp_add_param_s(iff, VX_IR_NAME_COND, (vx_IrValue) {.type = VX_IR_VAL_BLOCK,.block = cond});
    vx_IrOp_add_param_s(iff, VX_IR_NAME_COND_THEN, (vx_IrValue) {.type = VX_IR_VAL_BLOCK,.block = then});
    vx_IrOp_add_param_s(iff, VX_IR_NAME_COND_ELSE, (vx_IrValue) {.type = VX_IR_VAL_BLOCK,.block = els});

    vx_IrBlock_make_root(block, 7);

    return block;
}

static vx_IrBlock* build_test_bit(void) {
    vx_IrBlock* block = vx_IrBlock_init_heap(NULL, NULL);

    vx_IrVar a = 0, b = 1, o = 2, mask = 3;

    vx_IrBlock_add_in(block, a, ty_int);
    vx_IrBlock_add_in(block, b, ty_int);
    vx_IrBlock_add_out(block, o);

    {
        vx_IrOp* shift = vx_IrBlock_add_op_building(block);
        vx_IrOp_init(shift, VX_IR_OP_SHL, block);
        vx_IrOp_add_param_s(shift, VX_IR_NAME_OPERAND_A, (vx_IrValue) {.type = VX_IR_VAL_IMM_INT,.imm_int = 1});
        vx_IrOp_add_param_s(shift, VX_IR_NAME_OPERAND_B, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = b});
        vx_IrOp_add_out(shift, mask, ty_bool);
    }

    {
        vx_IrOp* and = vx_IrBlock_add_op_building(block);
        vx_IrOp_init(and, VX_IR_OP_AND, block);
        vx_IrOp_add_param_s(and, VX_IR_NAME_OPERAND_A, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = a});
        vx_IrOp_add_param_s(and, VX_IR_NAME_OPERAND_B, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = mask});
        vx_IrOp_add_out(and, o, ty_bool);
    }

    vx_IrBlock_make_root(block, 4);
    return block;
}

static vx_IrBlock* build_test_sum(void) {
    vx_IrBlock* block = vx_IrBlock_init_heap(NULL, NULL);
    vx_IrTypeRef ty_ptr = vx_ptrtype(block);

    vx_IrVar arr = 0, len = 1, idx = 2, l0 = 3, sum = 4, l1 = 5, l2 = 6, l3 = 7, l4 = 8;

    vx_IrBlock_add_in(block, arr, ty_ptr.ptr);
    vx_IrBlock_add_in(block, len, ty_int);

    {
        vx_IrOp* assign = vx_IrBlock_add_op_building(block);
        vx_IrOp_init(assign, VX_IR_OP_IMM, block);
        vx_IrOp_add_out(assign, sum, ty_int);
        vx_IrOp_add_param_s(assign, VX_IR_NAME_VALUE, (vx_IrValue) {.type = VX_IR_VAL_IMM_INT,.imm_int = 0});
    }

    vx_IrOp* loop = vx_IrBlock_add_op_building(block);
    vx_IrOp_init(loop, VX_CIR_OP_CFOR, block);

    {
        vx_IrBlock* start = vx_IrBlock_init_heap(block, loop);

        vx_IrOp* assign = vx_IrBlock_add_op_building(start);
        vx_IrOp_init(assign, VX_IR_OP_IMM, start);
        vx_IrOp_add_out(assign, idx, ty_int);
        vx_IrOp_add_param_s(assign, VX_IR_NAME_VALUE, (vx_IrValue) {.type = VX_IR_VAL_IMM_INT,.imm_int = 0});

        vx_IrOp_add_param_s(loop, VX_IR_NAME_LOOP_START, (vx_IrValue) {.type = VX_IR_VAL_BLOCK,.block = start});
    }

    {
        vx_IrBlock* cond = vx_IrBlock_init_heap(block, loop);

        vx_IrOp* cmp = vx_IrBlock_add_op_building(cond);
        vx_IrOp_init(cmp, VX_IR_OP_ULT, cond);
        vx_IrOp_add_out(cmp, l0, ty_bool);
        vx_IrOp_add_param_s(cmp, VX_IR_NAME_OPERAND_A, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = idx});
        vx_IrOp_add_param_s(cmp, VX_IR_NAME_OPERAND_B, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = len});

        vx_IrBlock_add_out(cond, l0);

        vx_IrOp_add_param_s(loop, VX_IR_NAME_COND, (vx_IrValue) {.type = VX_IR_VAL_BLOCK,.block = cond});
    }

    {
        vx_IrBlock* end = vx_IrBlock_init_heap(block, loop);

        vx_IrOp* inc = vx_IrBlock_add_op_building(end);
        vx_IrOp_init(inc, VX_IR_OP_ADD, end);
        vx_IrOp_add_out(inc, idx, ty_int);
        vx_IrOp_add_param_s(inc, VX_IR_NAME_OPERAND_A, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = idx});
        vx_IrOp_add_param_s(inc, VX_IR_NAME_OPERAND_B, (vx_IrValue) {.type = VX_IR_VAL_IMM_INT,.imm_int = 1});

        vx_IrOp_add_param_s(loop, VX_IR_NAME_LOOP_ENDEX, (vx_IrValue) {.type = VX_IR_VAL_BLOCK,.block = end});
    }

    {
        vx_IrBlock* body = vx_IrBlock_init_heap(block, loop);

        vx_IrOp* cast = vx_IrBlock_add_op_building(body);
        vx_IrOp_init(cast, VX_IR_OP_ZEROEXT, body);
        vx_IrOp_add_out(cast, l4, ty_ptr.ptr);
        vx_IrOp_add_param_s(cast, VX_IR_NAME_VALUE, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = idx});

        vx_IrOp* mul = vx_IrBlock_add_op_building(body);
        vx_IrOp_init(mul, VX_IR_OP_MUL, body);
        vx_IrOp_add_out(mul, l1, ty_ptr.ptr);
        vx_IrOp_add_param_s(mul, VX_IR_NAME_OPERAND_A, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = l4});
        vx_IrOp_add_param_s(mul, VX_IR_NAME_OPERAND_B, (vx_IrValue) {.type = VX_IR_VAL_IMM_INT,.imm_int = 4}); // TODO: sizeof op 

        vx_IrOp* add = vx_IrBlock_add_op_building(body);
        vx_IrOp_init(add, VX_IR_OP_ADD, body);
        vx_IrOp_add_out(add, l2, ty_ptr.ptr);
        vx_IrOp_add_param_s(add, VX_IR_NAME_OPERAND_A, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = arr});
        vx_IrOp_add_param_s(add, VX_IR_NAME_OPERAND_B, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = l1});

        vx_IrOp* load = vx_IrBlock_add_op_building(body);
        vx_IrOp_init(load, VX_IR_OP_LOAD, body);
        vx_IrOp_add_out(load, l3, ty_int);
        vx_IrOp_add_param_s(load, VX_IR_NAME_ADDR, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = l2});

        vx_IrOp* inc = vx_IrBlock_add_op_building(body);
        vx_IrOp_init(inc, VX_IR_OP_ADD, body);
        vx_IrOp_add_out(inc, sum, ty_int);
        vx_IrOp_add_param_s(inc, VX_IR_NAME_OPERAND_A, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = sum});
        vx_IrOp_add_param_s(inc, VX_IR_NAME_OPERAND_B, (vx_IrValue) {.type = VX_IR_VAL_VAR,.var = l3});

        vx_IrOp_add_param_s(loop, VX_IR_NAME_LOOP_DO, (vx_IrValue) {.type = VX_IR_VAL_BLOCK,.block = body});
    }

    vx_IrBlock_add_out(block, sum);
    vx_IrBlock_make_root(block, 9);
    return block;
}

static int cir_test(vx_IrBlock *block) {
    printf("Input:\n");
    vx_IrBlock_dump(block, stdout, 0);

    if (vx_cir_verify(block) != 0)
        return 1;

    vx_CIrBlock_fix(block); // TODO: why...
    vx_CIrBlock_normalize(block);
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

    printf("After SSA IR lower:\n");
    vx_IrBlock_dump(block, stdout, 0);

    vx_IrBlock_llir_fix_decl(block);
    opt_ll(block);

    printf("After LL IR opt:\n");
    vx_IrBlock_dump(block, stdout, 0);

    llir_prep_lower(block);

    printf("After LL IR lower prepare:\n");
    vx_IrBlock_dump(block, stdout, 0);

    printf("Generated code:\n");
    vx_cg_x86stupid_gen(block, stdout);

    vx_IrBlock_destroy(block);

    return 0;
}

int main(void) {
    vx_g_optconfig.if_eval = false;

    ty_int = vx_IrType_heap();
    ty_int->debugName = "i32";
    ty_int->kind = VX_IR_TYPE_KIND_BASE;
    ty_int->base.align = 4;
    ty_int->base.size = 4;

    ty_bool = vx_IrType_heap();
    ty_bool->debugName = "i8";
    ty_bool->kind = VX_IR_TYPE_KIND_BASE;
    ty_bool->base.align = 2; // depends on arch
    ty_bool->base.size = 1;

    printf("==== BOOL TEST ====\n");
    cir_test(build_test_bool());

    printf("==== CMOV TEST ====\n");
    cir_test(build_test_cmov());

    printf("===== BIT TEST ====\n");
    cir_test(build_test_bit());

    printf("===== SUM TEST ====\n");
    cir_test(build_test_sum());

    return 0;
}
