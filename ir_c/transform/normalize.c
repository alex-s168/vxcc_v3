#include <assert.h>
#include <stdlib.h>

#include "../cir.h"

/** transform cfor to c ir primitives that are representable in ssa ir */
void cirblock_normalize(CIRBlock *block) {
    assert(block->is_root);

    for (size_t i = 0; i < block->ins_len; i ++) {
        CIROp *op = block->ops;

        if (op->id == CIR_OP_CFOR) {
            CIRBlock *new = cirblock_heapalloc(block);

            CIRBlock *b_init = cirop_param(op, "init")->block;
            CIRBlock *b_cond = cirop_param(op, "cond")->block;
            CIRBlock *b_end = cirop_param(op, "end")->block;
            CIRBlock *b_do = cirop_param(op, "do")->block;
            // we free manually because we don't want to free blocks if should_free
            free(op->types);
            free(op->outs);
            cirop_remove_params(op);

            {
                CIROp opdo;
                cirop_init(&opdo, CIR_OP_FLATTEN_PLEASE);
                cirop_add_param_s(&opdo, "block", (CIRValue) { .type = CIR_VAL_BLOCK, .block = b_init });
                cirblock_add_op(new, &opdo);
            }

            CIROp opwhile;
            cirop_init(&opwhile, CIR_OP_WHILE);
            cirop_add_param_s(&opwhile, "cond", (CIRValue) { .type = CIR_VAL_BLOCK, .block = b_cond });

            CIRBlock *doblock = cirblock_heapalloc(new);
            {
                CIROp opdo;
                cirop_init(&opdo, CIR_OP_FLATTEN_PLEASE);
                cirop_add_param_s(&opdo, "block", (CIRValue) { .type = CIR_VAL_BLOCK, .block = b_do });
                cirblock_add_op(doblock, &opdo);
            }
            {
                CIROp opdo;
                cirop_init(&opdo, CIR_OP_FLATTEN_PLEASE);
                cirop_add_param_s(&opdo, "block", (CIRValue) { .type = CIR_VAL_BLOCK, .block = b_end });
                cirblock_add_op(doblock, &opdo);
            }

            cirop_add_param_s(&opwhile, "do", (CIRValue) { .type = CIR_VAL_BLOCK, .block = doblock });

            cirblock_add_op(new, &opwhile);

            op->id = CIR_OP_FLATTEN_PLEASE;
            cirop_add_param_s(op, "block", (CIRValue) { .type = CIR_VAL_BLOCK, .block = new });
        }
    }
}
