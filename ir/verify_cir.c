#include "verify.h"
#include "passes.h"

vx_Errors vx_CIrBlock_verify(vx_CU* cu, vx_IrBlock *block)
{
    vx_Errors errors;
    errors.items = NULL;
    errors.len = 0;

    vx_IrBlock_verify_ssa_based(&errors, block);

    return errors;
}
