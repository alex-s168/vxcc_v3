#include "opt.h"
#include "ir.h"

vx_OptConfig vx_g_optconfig = {
    .max_total_cmov_inline_cost = 4,
    .consteval_iterations = 6,
    .loop_simplify = true,
    .if_eval = true,
};

static void opt_pre(vx_IrBlock *block) {
    // place immediates into params
    vx_opt_inline_vars(vx_IrView_of_all(block), block);

    if (block->is_root)
        vx_opt_remove_vars(block);

    for (size_t i = 0; i < vx_g_optconfig.consteval_iterations; i ++) {
        // evaluate constants
        vx_opt_constant_eval(vx_IrView_of_all(block), block);
        // place results into params
        vx_opt_inline_vars(vx_IrView_of_all(block), block);
    }

    vx_opt_comparisions(vx_IrView_of_all(block), block);
}

void opt(vx_IrBlock *block) {
    // TODO: remove opt() calls from opt pass bodies

    for (size_t i = 0; i < block->ops_len; i ++) {
         vx_IrOp *op = &block->ops[i];

         for (size_t i = 0; i < op->params_len; i ++)
             if (op->params[i].val.type == VX_IR_VAL_BLOCK)
                 opt(op->params[i].val.block);
    }

    opt_pre(block);
    vx_opt_join_compute(vx_IrView_of_all(block), block);
    if (vx_g_optconfig.if_eval) {
        vx_opt_reduce_if(vx_IrView_of_all(block), block);
    }
    vx_opt_cmov(vx_IrView_of_all(block), block);
    opt_pre(block);
    if (vx_g_optconfig.loop_simplify) {
        vx_opt_reduce_loops(vx_IrView_of_all(block), block);
        vx_opt_loop_simplify(vx_IrView_of_all(block), block);
    }

    for (size_t i = 0; i < block->ops_len; i ++) {
        vx_IrOp *op = &block->ops[i];

        if (op->id == VX_IR_OP_FLATTEN_PLEASE) {
            vx_IrBlock *body = vx_IrOp_param(op, VX_IR_NAME_BLOCK)->block;
            opt(body);
        }
    }
}

void opt_ll(vx_IrBlock *block) {
    opt_pre(block);
    vx_opt_ll_jumps(vx_IrView_of_all(block), block);
}
