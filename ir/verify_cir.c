#include "cir.h"
#include "verify.h"

vx_Errors vx_CIrBlock_verify(vx_IrBlock *block)
{
    vx_Errors errors;
    errors.items = NULL;
    errors.len = 0;

    vx_IrBlock_verify_ssa_based(&errors, block);

    return errors;
}
