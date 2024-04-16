#include "../cir.h"

/**
 * Go trough every unconditional assignement in the block,
 * change the id of the vars to new,
 * unused vars and refactor every use AFTER the declaration;
 *
 * inside out!
 */
void cirblock_mksa_final(SsaBlock *block) {
    for (size_t i = 0; i < block->ops_len; i ++) {
        SsaOp *op = &block->ops[i];

        for (size_t j = 0; j < op->params_len; j ++)
            if (op->params[j].val.type == SSA_VAL_BLOCK)
                cirblock_mksa_final(op->params[j].val.block);

        for (size_t j = 0; j < op->outs_len; j ++) {
            SsaVar *var = &op->outs[j].var;
            const SsaVar old = *var;
            const SsaVar new = irblock_new_var(block, op);
            *var = new;
            SsaView view = irview_of_all(block);
            view.start = i + 1;
            irview_rename_var(view, block, old, new);
        }
    }
}
