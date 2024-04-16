#include <assert.h>
#include <stdlib.h>

#include "../cir.h"

void cirblock_normalize(SsaBlock *block) {
    assert(block->is_root);

    for (size_t i = 0; i < block->ins_len; i ++) {
        SsaOp *op = block->ops;

        if (op->id == CIR_OP_CFOR) {
            SsaBlock *new = irblock_heapalloc(block, i);

            SsaBlock *b_init = irop_param(op, SSA_NAME_LOOP_START)->block;
            SsaBlock *b_cond = irop_param(op, SSA_NAME_COND)->block;
            SsaBlock *b_end = irop_param(op, SSA_NAME_LOOP_ENDEX)->block;
            SsaBlock *b_do = irop_param(op, SSA_NAME_LOOP_DO)->block;
            // we free manually because we don't want to free blocks if should_free
            free(op->types); // TODO: ^ is stupid
            free(op->outs);
            free(op->states);
            irop_remove_params(op);

            {
                SsaOp opdo;
                irop_init(&opdo, SSA_OP_FLATTEN_PLEASE, new);
                irop_add_param_s(&opdo, SSA_NAME_BLOCK, (SsaValue) { .type = SSA_VAL_BLOCK, .block = b_init });
                irblock_add_op(new, &opdo);
            }

            SsaOp opwhile;
            irop_init(&opwhile, SSA_OP_WHILE, new);
            irop_add_param_s(&opwhile, SSA_NAME_COND, (SsaValue) { .type = SSA_VAL_BLOCK, .block = b_cond });

            SsaBlock *doblock = irblock_heapalloc(new, new->ops_len);
            {
                SsaOp opdo;
                irop_init(&opdo, SSA_OP_FLATTEN_PLEASE, new);
                irop_add_param_s(&opdo, SSA_NAME_BLOCK, (SsaValue) { .type = SSA_VAL_BLOCK, .block = b_do });
                irblock_add_op(doblock, &opdo);
            }
            {
                SsaOp opdo;
                irop_init(&opdo, SSA_OP_FLATTEN_PLEASE, new);
                irop_add_param_s(&opdo, SSA_NAME_BLOCK, (SsaValue) { .type = SSA_VAL_BLOCK, .block = b_end });
                irblock_add_op(doblock, &opdo);
            }

            irop_add_param_s(&opwhile, SSA_NAME_LOOP_DO, (SsaValue) { .type = SSA_VAL_BLOCK, .block = doblock });

            irblock_add_op(new, &opwhile);

            op->id = SSA_OP_FLATTEN_PLEASE;
            irop_add_param_s(op, SSA_NAME_BLOCK, (SsaValue) { .type = SSA_VAL_BLOCK, .block = new });
        }
    }
}
