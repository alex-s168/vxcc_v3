#include "../llir.h"
#include "../opt.h"

void vx_IrBlock_ll_finalize(vx_IrBlock *block, bool needEpilog) {
    if (needEpilog)
        return;

    vx_opt_ll_tailcall(block);
    vx_opt_ll_condtailcall(block);
}
