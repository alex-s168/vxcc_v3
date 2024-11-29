#include "../passes.h"

struct Move {
    vx_IrOp* what;
    vx_IrOp* before;
};

static void runSched(vx_IrBlock* block, struct Move **to_move, size_t *to_move_len,
        vx_IrOpFilter matchBegin, void *data0,
        vx_IrOpFilter matchEnd, void *data1)
{
    vx_IrOp* first = NULL;
    vx_IrOp* last = NULL;
    while (vx_IrBlock_nextOpListBetween(block, &first, &last,
                                        matchBegin, data0,
                                        matchEnd, data1))
    {
        assert(first); assert(last);

        if (!vx_IrBlock_allMatch(first, last, VX_IR_OPFILTER_PURE))
            continue;

        if (!vx_IrBlock_noneMatch(first, last, VX_IR_OPFILTER_ID(VX_IR_OP_LABEL)))
            continue;

        for (vx_IrOp* op = first->next; op; op = op->next)
        {
            if (op == last) break;

            vx_IrOp* beforeFirst = vx_IrOp_predecessor(first);
            if (beforeFirst == NULL) beforeFirst = block->first;

            bool ok = true;
            for (vx_IrOp* o = first->next; o && o != last; o = o->next) {
                FOR_INPUTS(op, inp, {
                    if (inp.type == VX_IR_VAL_VAR) {
                        if (vx_IrOp_varInOuts(o, inp.var)) {
                            ok = false;
                            break;
                        }
                    }
                });
            }

            if (ok && vx_IrOp_allDepsInRangeOrArgs(block, op, block->first, beforeFirst))
            {
                *to_move = realloc(*to_move, (*to_move_len + 1) * sizeof(struct Move));
				assert(*to_move);
                (*to_move)[(*to_move_len)++] = (struct Move) {
                    .what = op,
                    .before = first,
                };
            }
        }
    }
}

void vx_opt_ll_sched(vx_CU* cu, vx_IrBlock *block)
{
    assert(block->is_root);

    struct Move* to_move = NULL;
    size_t       to_move_len = 0;

    // TODO: this thing is broken af,
    runSched(block, &to_move, &to_move_len, VX_IR_OPFILTER_COMPARISION, VX_IR_OPFILTER_CONDITIONAL);
    // runSched(block, &to_move, &to_move_len, VX_IR_OPFILTER_PURE, VX_IR_OPFILTER_ID(VX_IR_OP_IMM)); // move imm ops as far up as possible

    for (size_t i = 0; i < to_move_len; i ++)
    {
        vx_IrOp* op = to_move[i].what;
        vx_IrOp* before = to_move[i].before;

        assert(op->parent == block);
        assert(before->parent == block);

        vx_IrOp_predecessor(op)->next = op->next;

        vx_IrOp* beforeBefore = vx_IrOp_predecessor(before);
        if (beforeBefore != NULL) {
            beforeBefore->next = op;
        } else {
            block->first = op;
        }

        op->next = before;
    }

    free(to_move);
}
