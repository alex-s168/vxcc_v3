#include "../passes.h"

void vx_IrBlock_ll_finalize(vx_CU* cu, vx_IrBlock *block, bool needEpilog)
{
    if (needEpilog)
        return;

    vx_opt_ll_tailcall(cu, block);
}
