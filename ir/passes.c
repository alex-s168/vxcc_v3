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

void vx_opt_preLower(vx_CU* cu, vx_IrBlock *block)
{
    RecCallInOut(opt_pre, cu, block);
    RecCallInOut(vx_opt_join_compute, cu, block);
    RecCallInOut(vx_opt_if_swapCases, cu, block);
    RecCallInOut(vx_IrBlock_ll_if_invert, cu, block);
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
	assert(block->is_root);
    vx_IrBlock_llir_fix_decl(cu, block);
	assert(block->is_root);
    //vx_IrBlock_llir_compact(block); /TODO?
    vx_IrBlock_lifetimes(cu, block);
    vx_IrBlock_ll_share_slots(cu, block);
    for (size_t i = 0; i < 2; i ++)
        vx_opt_ll_inlineVars(cu, block);
    vx_opt_ll_binary(cu, block);
    vx_IrBlock_llir_fix_decl(cu, block);
}

void vx_CIrBlock_fix(vx_CU* cu, vx_IrBlock* block) {
    vx_IrBlock* root = vx_IrBlock_root(block);

    for (size_t i = 0; i < block->ins_len; i ++)
        vx_IrBlock_putVar(root, block->ins[i].var, NULL);

    for (vx_IrOp* op = block->first; op; op = op->next) {
        for (size_t i = 0; i < op->outs_len; i ++)
            vx_IrBlock_putVar(root, op->outs[i].var, op);

        FOR_INPUTS(op, inp, {
           if (inp.type == VX_IR_VAL_BLOCK)
                vx_CIrBlock_fix(cu, inp.block);
        });
    }
}

void vx_IrBlock_llir_compact(vx_CU* cu, vx_IrBlock *root) {
    assert(root->is_root);

    for (vx_IrVar idx = 0; idx < root->as_root.vars_len; idx ++) {
        if (root->as_root.vars[idx].ll_type == NULL) {
            if (idx + 1 >= root->as_root.vars_len) break;
            for (vx_IrVar after = idx + 1; after < root->as_root.vars_len; after ++) {
                assert(root != NULL && root->is_root);
                vx_IrBlock_renameVar(root, after, after - 1, VX_RENAME_VAR_BOTH);
            }
        }
    }
}

void vx_IrBlock_llir_fix_decl(vx_CU* cu, vx_IrBlock *root) {
    assert(root->is_root);

    // TODO: WHY THE FUCK IS THIS EVEN REQUIRED????

	memset(root->as_root.labels, 0, sizeof(*root->as_root.labels) * root->as_root.labels_len);

	for (size_t i = 0; i < root->as_root.vars_len; i ++) {
		root->as_root.vars[i].decl = NULL;
		root->as_root.vars[i].ll_type = NULL;
		root->as_root.vars[i].ever_placed = false;
	}

    for (size_t i = 0; i < root->ins_len; i ++) {
        vx_IrVar v = root->ins[i].var;
        assert(v < root->as_root.vars_len);
        root->as_root.vars[v].ll_type = root->ins[i].type;
    }

    for (vx_IrOp *op = root->first; op; op = op->next) {
        for (size_t j = 0; j < op->outs_len; j ++) {
            vx_IrTypedVar out = op->outs[j];
            if (vx_IrType_size(cu, root, out.type) == 0) {
                fprintf(stderr, "size of type %s in 0", out.type->debugName);
                exit(1);
            }
            assert(out.var < root->as_root.vars_len);
            vx_IrOp **decl = &root->as_root.vars[out.var].decl;
            if (*decl == NULL) {
                *decl = op;
                root->as_root.vars[out.var].ll_type = out.type;
            }
        }

        if (op->id == VX_IR_OP_LABEL) {
            size_t id = vx_IrOp_param(op, VX_IR_NAME_ID)->id;
            assert(id < root->as_root.labels_len);
            vx_IrOp **decl = &root->as_root.labels[id].decl;
            if (*decl == NULL) {
                *decl = op;
            }
        }
    }

    for (vx_IrOp* op = root->first; op; op = op->next) {
        if (op->id == VX_IR_OP_PLACE) {
            vx_IrValue val = *vx_IrOp_param(op, VX_IR_NAME_VAR);
            assert(val.type == VX_IR_VAL_VAR && "inliner fucked up (VX_IR_OP_PLACE)");
            root->as_root.vars[val.var].ever_placed = true;
        }
    }
}
