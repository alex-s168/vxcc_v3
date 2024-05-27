#include "ir.h"

void vx_IrBlock_llir_lower(vx_IrBlock *block);

void vx_IrBlock_llir_fix_decl(vx_IrBlock *root);

/** block needs to be 100% flat */
void vx_IrBlock_lifetimes(vx_IrBlock *block);

// block needs to be 100% flat, decl of vars must be known, decl won't be known after this fn anymore; adds type info to vars
void vx_IrBlock_ll_share_slots(vx_IrBlock *block);

static void llir_prep_lower(vx_IrBlock *block) {
    vx_IrBlock_llir_fix_decl(block);
    vx_IrBlock_lifetimes(block);
    vx_IrBlock_ll_share_slots(block);
}
