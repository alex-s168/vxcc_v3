#include "../ir.h"

void vx_IrBlock_regalloc(vx_IrBlock * block) {
    for (size_t i = 0; i < block->as_root.vars_len; i ++) {
        lifetime l = block->as_root.vars[i].ll_lifetime;
        if (l.first == NULL)
            continue;

        // TODO
    }
}

