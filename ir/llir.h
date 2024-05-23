#include "ir.h"

void vx_IrBlock_llir_lower(vx_IrBlock *block);

/** block needs to be 100% fat */
void vx_IrBlock_lifetimes(vx_IrBlock *block);

// block needs to be 100% flat, decl of vars must be known, decl won't be known after this fn anymore; adds type info to vars
void vx_IrBlock_ll_share_slots(vx_IrBlock *block);
