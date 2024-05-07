#ifndef CIR_H
#define CIR_H

#include "ir.h"

/** transform cfor to c ir primitives that are representable in ir ir */
void vx_CIrBlock_normalize(vx_IrBlock *block);

vx_OptIrVar vx_CIrBlock_mksa_states(vx_IrBlock *block);
void vx_CIrBlock_mksa_final(vx_IrBlock *block);

vx_Errors vx_CIrBlock_verify(const vx_IrBlock *block, vx_OpPath path);

static int vx_cir_verify(const vx_IrBlock *block) {
    vx_OpPath path;
    path.ids = NULL;
    path.len = 0;
    const vx_Errors errs = vx_CIrBlock_verify(block, path);
    vx_Errors_print(errs, stderr);
    vx_Errors_free(errs);
    return errs.len > 0;
}

#endif //CIR_H
