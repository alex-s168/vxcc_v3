#include "passes.h"
#include "ir.h"

static void RecCallInOut(void (*fn)(vx_CU*, vx_IrBlock*), vx_CU* cu, vx_IrBlock* block)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        FOR_INPUTS(op, input, ({
            if (input.type == VX_IR_VAL_BLOCK)
                RecCallInOut(fn, cu, input.block);
        }));
    }

    fn(cu, block);
}

static void opt_pre(vx_CU* cu, vx_IrBlock *block) {
    // place immediates into params
    vx_opt_inline_vars(cu, block);

    vx_opt_vars(cu, block);

    for (size_t i = 0; i < cu->opt.consteval_iterations; i ++) {
        // evaluate constants
        vx_opt_constant_eval(cu, block);
        // place results into params
        vx_opt_inline_vars(cu, block);
    }

    vx_opt_simple_patterns(cu, block);
    vx_opt_comparisions(cu, block);
}

void vx_opt(vx_CU* cu, vx_IrBlock *block) {
    assert(block != NULL);

    for (vx_IrOp *op = block->first; op; op = op->next) {
        FOR_INPUTS(op, input, ({
            if (input.type == VX_IR_VAL_BLOCK)
                vx_opt(cu, input.block);
        }));
    }

    opt_pre(cu, block);
    vx_opt_join_compute(cu, block);
    if (cu->opt.if_eval) {
        vx_opt_reduce_if(cu, block);
        vx_opt_if_opts(cu, block);
    }
    vx_opt_cmov(cu, block);
    opt_pre(cu, block);
    if (cu->opt.loop_simplify) {
        vx_opt_reduce_loops(cu, block);
        vx_opt_loop_simplify(cu, block);
    }
    opt_pre(cu, block);
}

static void opt_preLower(vx_CU* cu, vx_IrBlock *block)
{
    RecCallInOut(opt_pre, cu, block);
    RecCallInOut(vx_opt_join_compute, cu, block);
    RecCallInOut(vx_opt_if_swapCases, cu, block);
    RecCallInOut(vx_opt_comparisions, cu, block);
    RecCallInOut(opt_pre, cu, block);
}

void vx_opt_ll(vx_CU* cu, vx_IrBlock *block) {
    vx_opt_ll_dce(cu, block);
    for (size_t i = 0; i < cu->opt.consteval_iterations; i ++) {
        vx_opt_ll_inlineVars(cu, block);
        vx_opt_vars(cu, block);
    }

    vx_IrBlock_ll_cmov_expand(cu, block); // this makes it non-ssa but ll_sched can handle it (but only because it's cmov!!!)
    vx_opt_ll_sched(cu, block);

    for (size_t i = 0; i < 4; i ++) {
        vx_opt_ll_jumps(cu, block);
        vx_opt_ll_dce(cu, block);
    }

    for (size_t i = 0; i < cu->opt.consteval_iterations; i ++) {
        vx_opt_ll_inlineVars(cu, block);
        vx_opt_vars(cu, block);
    }
}

void vx_llir_prep_lower(vx_CU* cu, vx_IrBlock *block) {
    vx_IrBlock_llir_fix_decl(cu, block);
    //vx_IrBlock_llir_compact(block); /TODO?
    vx_IrBlock_lifetimes(cu, block);
    vx_IrBlock_ll_share_slots(cu, block);
    for (size_t i = 0; i < 2; i ++)
        vx_opt_ll_inlineVars(cu, block);
    vx_opt_ll_binary(cu, block);
    vx_IrBlock_llir_fix_decl(cu, block);
}
