#ifndef CIR_H
#define CIR_H

#include "ir.h"

/** transform cfor to c ir primitives that are representable in ir ir */
TRANSFORM_PASS void vx_CIrBlock_normalize(vx_IrBlock *);
TRANSFORM_PASS vx_OptIrVar vx_CIrBlock_mksa_states(vx_IrBlock *);
TRANSFORM_PASS void vx_CIrBlock_mksa_final(vx_IrBlock *);

void vx_CIrBlock_fix(vx_IrBlock*);

vx_Errors vx_CIrBlock_verify(vx_IrBlock *block);

// returns last var that was originally onVar 
vx_IrVar vx_CIrBlock_partial_mksaFinal_norec(vx_IrBlock* block, vx_IrVar onVar);

static int vx_cir_verify(vx_IrBlock *block) {
    const vx_Errors errs = vx_CIrBlock_verify(block);
    vx_Errors_print(errs, stderr);
    vx_Errors_free(errs);
    return errs.len > 0;
}

#endif //CIR_H
