#include "../passes.h"

// TODO: move to ir/transform.c
static vx_IrVar genIntoVar(vx_IrBlock* block, vx_IrValue val)
{
    vx_IrOp* op = vx_IrBlock_addOpBuilding(block);
    vx_IrOp_init(op, VX_IR_OP_IMM, block);
    vx_IrBlock* root = vx_IrBlock_root(block);
    vx_IrVar var = vx_IrBlock_newVar(root, op);
    vx_IrTypeRef ty = vx_IrValue_type(root, val);
    vx_IrOp_addOut(op, var, ty.ptr);
    vx_IrTypeRef_drop(ty);
    vx_IrOp_addParam_s(op, VX_IR_NAME_VALUE, val);
    return var;
}

void vx_opt_if_opts(vx_CU* cu, vx_IrBlock* block)
{
    // move code after if statements that partially end flow into the branch that doesn't end flow
    for (vx_IrOp* op = block->first; op; op = op->next)
    {
        if (op->id != VX_IR_OP_IF)
            continue;

        vx_IrBlock* bthen = vx_IrOp_param(op, VX_IR_NAME_COND_THEN)->block; assert(bthen);
        vx_IrBlock* belse = vx_IrOp_param(op, VX_IR_NAME_COND_ELSE)->block; assert(belse);

        bool thenEnds = vx_IrBlock_endsFlow(bthen); 
        bool elseEnds = vx_IrBlock_endsFlow(belse);

        if (thenEnds == elseEnds)
            continue;

        vx_IrBlock* notEndingBlk = thenEnds ? belse : bthen;

        vx_IrOp* next = op->next;
        op->next = NULL;

        for (vx_IrOp* o = next; o; o = o->next)
            o->parent = notEndingBlk;

        {
            vx_IrOp* tail = vx_IrBlock_tail(notEndingBlk);
            if (tail == NULL) {
                notEndingBlk->first = next;
            } else {
                tail->next = next;
            }
        }

        for (size_t i = 0; i < op->outs_len; i ++)
            vx_IrBlock_renameVar(notEndingBlk, op->outs[i].var, notEndingBlk->outs[i], VX_RENAME_VAR_BOTH);

        break; // there is no next anymore 
    }

    vx_opt_ll_dce(cu, block); // it says ll but also works in ssa 

    for (vx_IrOp* op = block->first; op; op = op->next)
    {
        if (op->id != VX_IR_OP_IF)
            continue;

        vx_IrBlock* bthen = vx_IrOp_param(op, VX_IR_NAME_COND_THEN)->block;
        vx_IrBlock* belse = vx_IrOp_param(op, VX_IR_NAME_COND_ELSE)->block;

        vx_IrOp* thenLast = vx_IrBlock_tail(bthen);
        vx_IrOp* elseLast = vx_IrBlock_tail(belse);

        if (!thenLast) continue;
        if (!elseLast) continue;
        if (thenLast->id != VX_IR_OP_RETURN) continue;
        if (elseLast->id != VX_IR_OP_RETURN) continue;

        vx_IrOp* retInst = vx_IrBlock_insertOpCreateAfter(block, op, VX_IR_OP_RETURN);

        vx_IrBlock* root = vx_IrBlock_root(block);
        size_t numRets = root->outs_len;
        for (size_t i = 0; i < numRets; i ++)
        {
            vx_IrBlock_addOut(bthen, genIntoVar(bthen, thenLast->args[i]));
            vx_IrBlock_addOut(belse, genIntoVar(belse, elseLast->args[i]));

            vx_IrVar va = vx_IrBlock_newVar(root, op);
            vx_IrOp_addOut(op, va, vx_IrBlock_typeofVar(root, root->outs[i]));
            vx_IrOp_addArg(retInst, VX_IR_VALUE_VAR(va));
        }

        vx_IrOp_remove(thenLast);
        vx_IrOp_remove(elseLast);

        break; // the next op is going to be the return we just inserted
    }
}
