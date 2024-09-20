#include "ir.h"
#include "opt.h"

#ifndef _LLIR_H
#define _LLIR_H

OPT_PASS void vx_opt_ll_binary(vx_IrBlock *block);

OPT_PASS void vx_opt_ll_tailcall(vx_IrBlock *block);

void vx_IrBlock_llir_preLower_loops(vx_IrBlock *block);

void vx_IrBlock_llir_lower(vx_IrBlock *block);

void vx_IrBlock_llir_fix_decl(vx_IrBlock *root);

void vx_IrBlock_llir_compact(vx_IrBlock *root);

/** block needs to be 100% flat */
void vx_IrBlock_lifetimes(vx_IrBlock *block);

// block needs to be 100% flat, decl of vars must be known, decl won't be known after this fn anymore; adds type info to vars
void vx_IrBlock_ll_share_slots(vx_IrBlock *block);

void vx_IrBlock_ll_cmov_expand(vx_IrBlock *block);

// called by the codegen after it knows if it needs epilog
void vx_IrBlock_ll_finalize(vx_IrBlock *block, bool needEpilog);

static void llir_prep_lower(vx_IrBlock *block) {
    vx_IrBlock_llir_fix_decl(block);
    //vx_IrBlock_llir_compact(block); /TODO?
    vx_IrBlock_lifetimes(block);
    vx_IrBlock_ll_share_slots(block);
    for (size_t i = 0; i < 2; i ++)
        vx_opt_ll_inlineVars(block);
    vx_opt_ll_binary(block);
    vx_IrBlock_llir_fix_decl(block);
}

#endif //_LLIR_H
