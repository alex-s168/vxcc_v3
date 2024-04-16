#include "../opt.h"

static void trav(SsaOp *op, void *data) {
    SsaBlock *block = data;

    if (op->id != SSA_OP_IMM)
        return;

    const SsaValue value = *irop_param(op, SSA_NAME_VALUE);

    for (size_t i = 0; i < op->outs_len; i ++) {
        const SsaVar out = op->outs[i].var;
        irview_substitude_var(irview_of_all(block), block, out, value);
    }
}

void opt_inline_vars(SsaView view, SsaBlock *block) {
    irview_deep_traverse(view, trav, block);
}
