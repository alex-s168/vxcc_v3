#include "../cir.h"

/**
 * Go trough every unconditional assignement in the block,
 * change the id of the vars to new,
 * unused vars and refactor every use AFTER the declaration;
 *
 * inside out!
 */
void cirblock_mksa_final(CIRBlock *block) {
    for (size_t i = 0; i < block->ops_len; i ++) {
        const CIROp *op = &block->ops[i];

        for (size_t j = 0; j < op->params_len; j ++)
            if (op->params[j].val.type == CIR_VAL_BLOCK)
                cirblock_mksa_final(op->params[j].val.block);

        for (size_t j = 0; j < op->outs_len; j ++) {
            CIRVar *var = &op->outs[j].var;
            const CIRVar old = *var;
            const CIRVar new = cirblock_new_var(block);
            *var = new;
            cirblock_rename_var_after(block, i + 1, old, new);
        }
    }
}