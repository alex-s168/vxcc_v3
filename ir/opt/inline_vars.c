#include "../opt.h"


struct vx_IrView_substitute_var__data {
    vx_IrBlock *block;
    vx_IrVar old;
    vx_IrValue new;
    vx_IrOp* newSrc;
};

static bool vx_IrView_substitute_var__trav(vx_IrOp *op, void *dataIn) {
    struct vx_IrView_substitute_var__data *data = dataIn;

    vx_IrOp *first = op;
    vx_IrOp *last = data->newSrc;
    vx_IrOp_earlierFirst(&first, &last);

    for (vx_IrOp* o = first->next; o; o = o->next) {
        if (vx_IrOp_after(o, last)) break;

        if (o->id == VX_IR_OP_LABEL) {
            return false;
        }
    } 

    vx_IrBlock* block = op->parent;
    for (size_t j = 0; j < block->outs_len; j ++) {
        if (block->outs[j] == data->old) {
            if (data->new.type == VX_IR_VAL_VAR) {
                block->outs[j] = data->new.var;
            }
        }
    }

    for (size_t i = 0; i < op->params_len; i ++)
        if (op->params[i].val.type == VX_IR_VAL_VAR && op->params[i].val.var == data->old)
            op->params[i].val = data->new;

    return false;
}

static void vx_IrBlock_substitute_var(vx_IrBlock *block, vx_IrVar old, vx_IrValue new, vx_IrOp* newSrc) {
    struct vx_IrView_substitute_var__data data = { .block = block, .old = old, .new = new, .newSrc = newSrc };
    vx_IrBlock_deepTraverse(block, vx_IrView_substitute_var__trav, &data);
}

// =======================================================================

static bool trav (vx_IrOp *op, void *data)
{
    vx_IrBlock *block = data;

    if (op->id != VX_IR_OP_IMM)
        return false;

    const vx_IrValue value = *vx_IrOp_param(op, VX_IR_NAME_VALUE);

    for (size_t i = 0; i < op->outs_len; i ++) {
        const vx_IrVar out = op->outs[i].var;
        vx_IrBlock_substitute_var(block, out, value, op);
    }

    return false;
}

void vx_opt_inline_vars(vx_IrBlock *block) {
    vx_IrBlock_deepTraverse(block, trav, block);
}

void vx_opt_ll_inlineVars(vx_IrBlock *block)
{
    for (vx_IrOp* imm = block->first; imm; imm = imm->next)
    {
        if (imm->id != VX_IR_OP_IMM)
            continue;

        vx_IrVar oVar = imm->outs[0].var;

        vx_IrValue nVal = *vx_IrOp_param(imm, VX_IR_NAME_VALUE);

        if (nVal.type == VX_IR_VAL_VAR) {
            vx_IrVar nVar = nVal.var;
            if (nVar == oVar) {
                vx_IrOp* pred = vx_IrOp_predecessor(imm);
                if (pred == NULL) {
                    block->first = imm->next;
                } else {
                    pred->next = imm->next;
                }
                continue;
            }
        }

        for (vx_IrOp* next = imm->next; next; next = next->next) {
            if (next->id == VX_IR_OP_LABEL)
                break;

            FOR_INPUTS_REF(next, inpr, {
                if (inpr->type == VX_IR_VAL_VAR && inpr->var == oVar) {
                    *inpr = nVal;
                }
            });


            if (nVal.type == VX_IR_VAL_VAR) {
                vx_IrVar nVar = nVal.var;
                if (vx_IrOp_varInOuts(next, nVar))
                    break;
            }

            if (vx_IrOp_varInOuts(next, oVar)) 
                break;

            if (nVal.type == VX_IR_VAL_VAR) {
                vx_IrVar nVar = nVal.var;
                if (next->next == NULL) // we are at tail 
                    for (size_t i = 0; i < block->outs_len; i ++) 
                        if (block->outs[i] == oVar)
                            block->outs[i] = nVar;
            }
        }
    }
}
