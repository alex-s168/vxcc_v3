#include "opt.h"
#include "ir.h"
#include "llir.h"

vx_OptConfig vx_g_optconfig = {
    .max_total_cmov_inline_cost = 4,
    .consteval_iterations = 6,
    .loop_simplify = true,
    .if_eval = true,
};

static void RecCallInOut(void (*fn)(vx_IrBlock*), vx_IrBlock* block)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        FOR_INPUTS(op, input, ({
            if (input.type == VX_IR_VAL_BLOCK)
                RecCallInOut(fn, input.block);
        }));
    }

    fn(block);
}

static void opt_pre(vx_IrBlock *block) {
    // place immediates into params
    vx_opt_inline_vars(block);

    vx_opt_vars(block);

    for (size_t i = 0; i < vx_g_optconfig.consteval_iterations; i ++) {
        // evaluate constants
        vx_opt_constant_eval(block);
        // place results into params
        vx_opt_inline_vars(block);
    }

    vx_opt_simple_patterns(block);
    vx_opt_comparisions(block);
}

void opt(vx_IrBlock *block) {
    assert(block != NULL);

    for (vx_IrOp *op = block->first; op; op = op->next) {
        FOR_INPUTS(op, input, ({
            if (input.type == VX_IR_VAL_BLOCK)
                opt(input.block);
        }));
    }

    opt_pre(block);
    vx_opt_join_compute(block);
    if (vx_g_optconfig.if_eval) {
        vx_opt_reduce_if(block);
        vx_opt_if_opts(block);
    }
    vx_opt_cmov(block);
    opt_pre(block);
    if (vx_g_optconfig.loop_simplify) {
        vx_opt_reduce_loops(block);
        vx_opt_loop_simplify(block);
    }
    opt_pre(block);
}

void opt_preLower(vx_IrBlock *block)
{
    RecCallInOut(opt_pre, block);
    RecCallInOut(vx_opt_join_compute, block);
    RecCallInOut(vx_opt_if_swapCases, block);
    RecCallInOut(vx_opt_comparisions, block);
    RecCallInOut(opt_pre, block);
}

void opt_ll(vx_IrBlock *block) {
    vx_opt_ll_dce(block);
    for (size_t i = 0; i < vx_g_optconfig.consteval_iterations; i ++) {
        vx_opt_ll_inlineVars(block);
        vx_opt_vars(block);
    }

    vx_IrBlock_ll_cmov_expand(block); // this makes it non-ssa but ll_sched can handle it (but only because it's cmov!!!)
    vx_opt_ll_sched(block);

    for (size_t i = 0; i < 4; i ++) {
        vx_opt_ll_jumps(block);
        vx_opt_ll_dce(block);
    }

    for (size_t i = 0; i < vx_g_optconfig.consteval_iterations; i ++) {
        vx_opt_ll_inlineVars(block);
        vx_opt_vars(block);
    }
}
