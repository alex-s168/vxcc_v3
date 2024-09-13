#include "../opt.h"

void vx_opt_ll_sched(vx_IrBlock *block) {
    assert(block->is_root);

    vx_IrBlock_dump(block, stdout, 0);

    struct Move {
        vx_IrOp* what;
        vx_IrOp* before;
    };

    struct Move* to_move = NULL;
    size_t       to_move_len = 0;

    vx_IrOp* first = NULL;
    vx_IrOp* last = NULL;
    while (vx_IrBlock_nextOpListBetween(block, &first, &last,
                                        VX_IR_OPFILTER_COMPARISION,
                                        VX_IR_OPFILTER_CONDITIONAL))
    {
        assert(first); assert(last);

        if (!vx_IrBlock_allMatch(first, last, VX_IR_OPFILTER_PURE)) {
            continue;
        }

        for (vx_IrOp* op = first->next; op; op = op->next)
        {
            if (op == last) break;

            vx_IrOp* beforeFirst = vx_IrOp_predecessor(first);
            if (beforeFirst == NULL) beforeFirst = block->first;
            if (vx_IrOp_allDepsInRangeOrArgs(block, op, block->first, beforeFirst))
            {
                to_move = realloc(to_move, (to_move_len + 1) * sizeof(struct Move));
                to_move[to_move_len++] = (struct Move) {
                    .what = op,
                    .before = first,
                };
            }
        }
    }
    
    for (size_t i = 0; i < to_move_len; i ++)
    {
        vx_IrOp* op = to_move[i].what;
        vx_IrOp* before = to_move[i].before;

        assert(op->parent == block);
        assert(before->parent == block);

        vx_IrOp_predecessor(op)->next = op->next;

        vx_IrOp* beforeBefore = vx_IrOp_predecessor(before);
        vx_IrOp_dump(beforeBefore, stdout, 0);
        if (beforeBefore != NULL) {
            beforeBefore->next = op;
        } else {
            block->first = op;
        }

        op->next = before;
    }

    free(to_move);

    vx_IrBlock_dump(block, stdout, 0);


}
