#ifndef VX_PASSES_H
#define VX_PASSES_H

#include "ir.h"

void vx_opt_loop_simplify(vx_CU* cu, vx_IrBlock *block);
void vx_opt_comparisions(vx_CU* cu, vx_IrBlock *block);
void vx_opt_constant_eval(vx_CU* cu, vx_IrBlock *block);
void vx_opt_inline_vars(vx_CU* cu, vx_IrBlock *block);
void vx_opt_join_compute(vx_CU* cu, vx_IrBlock *block);
void vx_opt_reduce_if(vx_CU* cu, vx_IrBlock *block);
void vx_opt_reduce_loops(vx_CU* cu, vx_IrBlock *block);
void vx_opt_cmov(vx_CU* cu, vx_IrBlock *block);
void vx_opt_vars(vx_CU* cu, vx_IrBlock *block);
void vx_opt_simple_patterns(vx_CU* cu, vx_IrBlock* block);
void vx_opt_if_opts(vx_CU* cu, vx_IrBlock* block);
void vx_opt_if_swapCases(vx_CU* cu, vx_IrBlock* block);

void vx_opt_ll_jumps(vx_CU* cu, vx_IrBlock *block);
void vx_opt_ll_inlineVars(vx_CU* cu, vx_IrBlock *block);
void vx_opt_ll_dce(vx_CU* cu, vx_IrBlock *block);
void vx_opt_ll_condtailcall(vx_CU* cu, vx_IrBlock *block);
void vx_opt_ll_sched(vx_CU* cu, vx_IrBlock *block);
void vx_opt_ll_binary(vx_CU* cu,vx_IrBlock *block);
void vx_opt_ll_tailcall(vx_CU* cu,vx_IrBlock *block);

void vx_opt(vx_CU* cu, vx_IrBlock *block);
void vx_opt_preLower(vx_CU* cu, vx_IrBlock *block);
void vx_opt_ll(vx_CU* cu, vx_IrBlock *block);

void vx_IrBlock_llir_preLower_loops(vx_CU* cu,vx_IrBlock *block);
void vx_IrBlock_llir_preLower_ifs(vx_CU* cu,vx_IrBlock *block);
void vx_IrBlock_llir_lower(vx_CU* cu,vx_IrBlock *block);
void vx_IrBlock_llir_fix_decl(vx_CU* cu,vx_IrBlock *root);
void vx_IrBlock_llir_compact(vx_CU* cu,vx_IrBlock *root);
/** block needs to be 100% flat */
void vx_IrBlock_lifetimes(vx_CU* cu,vx_IrBlock *block);
// block needs to be 100% flat, decl of vars must be known, decl won't be known after this fn anymore; adds type info to vars
void vx_IrBlock_ll_share_slots(vx_CU* cu,vx_IrBlock *block);
void vx_IrBlock_ll_cmov_expand(vx_CU* cu,vx_IrBlock *block);
// called by the codegen after it knows if it needs epilog
void vx_IrBlock_ll_finalize(vx_CU* cu, vx_IrBlock *block, bool needEpilog);
void vx_llir_prep_lower(vx_CU* cu, vx_IrBlock *block);

void vx_CIrBlock_normalize(vx_CU*, vx_IrBlock *);
vx_OptIrVar vx_CIrBlock_mksa_states(vx_CU*, vx_IrBlock *);
void vx_CIrBlock_mksa_final(vx_CU*, vx_IrBlock *);
void vx_CIrBlock_fix(vx_CU*, vx_IrBlock*);

// TODO: move verify into ir.h
vx_Errors vx_CIrBlock_verify(vx_CU*, vx_IrBlock *block);
static int vx_cir_verify(vx_CU* cu, vx_IrBlock *block) {
    const vx_Errors errs = vx_CIrBlock_verify(cu, block);
    vx_Errors_print(errs, stderr);
    vx_Errors_free(errs);
    return errs.len > 0;
}

#endif
