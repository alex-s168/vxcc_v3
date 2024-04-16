#include <assert.h>
#include <stdlib.h>

#include "../cir.h"

void cirblock_normalize(SsaBlock *block) {
    assert(block->is_root);

    for (size_t i = 0; i < block->ins_len; i ++) {
        SsaOp *op = block->ops;

        if (op->id == CIR_OP_CFOR) {
            SsaBlock *new = ssablock_heapalloc(block, i);

            SsaBlock *b_init = ssaop_param(op, SSA_NAME_LOOP_START)->block;
            SsaBlock *b_cond = ssaop_param(op, SSA_NAME_COND)->block;
            SsaBlock *b_end = ssaop_param(op, SSA_NAME_LOOP_ENDEX)->block;
            SsaBlock *b_do = ssaop_param(op, SSA_NAME_LOOP_DO)->block;
            // we free manually because we don't want to free blocks if should_free
            free(op->types); // TODO: ^ is stupid
            free(op->outs);
            free(op->states);
            ssaop_remove_params(op);

            {
                SsaOp opdo;
                ssaop_init(&opdo, SSA_OP_FLATTEN_PLEASE, new);
                ssaop_add_param_s(&opdo, SSA_NAME_BLOCK, (SsaValue) { .type = SSA_VAL_BLOCK, .block = b_init });
                ssablock_add_op(new, &opdo);
            }

            SsaOp opwhile;
            ssaop_init(&opwhile, SSA_OP_WHILE, new);
            ssaop_add_param_s(&opwhile, SSA_NAME_COND, (SsaValue) { .type = SSA_VAL_BLOCK, .block = b_cond });

            SsaBlock *doblock = ssablock_heapalloc(new, new->ops_len);
            {
                SsaOp opdo;
                ssaop_init(&opdo, SSA_OP_FLATTEN_PLEASE, new);
                ssaop_add_param_s(&opdo, SSA_NAME_BLOCK, (SsaValue) { .type = SSA_VAL_BLOCK, .block = b_do });
                ssablock_add_op(doblock, &opdo);
            }
            {
                SsaOp opdo;
                ssaop_init(&opdo, SSA_OP_FLATTEN_PLEASE, new);
                ssaop_add_param_s(&opdo, SSA_NAME_BLOCK, (SsaValue) { .type = SSA_VAL_BLOCK, .block = b_end });
                ssablock_add_op(doblock, &opdo);
            }

            ssaop_add_param_s(&opwhile, SSA_NAME_LOOP_DO, (SsaValue) { .type = SSA_VAL_BLOCK, .block = doblock });

            ssablock_add_op(new, &opwhile);

            op->id = SSA_OP_FLATTEN_PLEASE;
            ssaop_add_param_s(op, SSA_NAME_BLOCK, (SsaValue) { .type = SSA_VAL_BLOCK, .block = new });
        }
    }
}
