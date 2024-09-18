#include "../opt.h"

static vx_IrVar block_res_as_var(vx_IrBlock *parent, vx_IrBlock *block) {
    vx_IrBlock* root = vx_IrBlock_root(parent);
    assert(root);

    vx_IrOp *op = vx_IrBlock_addOpBuilding(parent);
    vx_IrOp_init(op, VX_IR_OP_FLATTEN_PLEASE, parent);
    vx_IrVar dest = vx_IrBlock_newVar(parent, op);
    vx_IrOp_addParam_s(op, VX_IR_NAME_BLOCK, VX_IR_VALUE_BLK(block));
    assert(block->outs_len >= 1);
    vx_IrType* type = vx_IrBlock_typeofVar(block, block->outs[0]);
    assert(type);
    vx_IrOp_addOut(op, dest, type);
    return dest;
}

//   convert:
// DEST = if cond=COND, then={
//     ^ THEN: non volatile
// }, else={
//     ^ ELSE: non volatile
// }
//
//   to:
// V0 < THEN 
// V1 < ELSE
// DEST = cmov cond=COND, then=V0, else=V1
//
//   if:
// cost of then + else is small enough
// exactly one out

void vx_opt_cmov(vx_IrBlock *block)
{
    vx_IrBlock* root = vx_IrBlock_root(block);
    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->id != VX_IR_OP_IF)
            continue;

        if (op->outs_len != 1)
            continue;

        vx_IrValue *pthen = vx_IrOp_param(op, VX_IR_NAME_COND_THEN);
        if (!pthen)
            continue;

        vx_IrValue *pelse = vx_IrOp_param(op, VX_IR_NAME_COND_ELSE);
        if (!pelse)
            continue;

        assert(pthen->type == VX_IR_VAL_BLOCK);
        vx_IrBlock *then = pthen->block;
        if (vx_IrBlock_isVolatile(then))
            continue;

        assert(pelse->type == VX_IR_VAL_BLOCK);
        vx_IrBlock *els = pelse->block;
        if (vx_IrBlock_isVolatile(els))
            continue;

        size_t total_cost = vx_IrBlock_inlineCost(then) + vx_IrBlock_inlineCost(els);
        if (total_cost > vx_g_optconfig.max_total_cmov_inline_cost)
            continue;

        vx_IrValue condv = *vx_IrOp_param(op, VX_IR_NAME_COND);
        assert(condv.type == VX_IR_VAL_BLOCK);
        vx_IrBlock *cond = condv.block;

        vx_IrBlock *body = vx_IrBlock_initHeap(block, op);

        vx_IrVar v0 = block_res_as_var(body, then);
        vx_IrVar v1 = block_res_as_var(body, els);

        vx_IrOp *cmov = vx_IrBlock_addOpBuilding(body);
        vx_IrOp_init(cmov, VX_IR_OP_CMOV, body);
        vx_IrVar dest = vx_IrBlock_newVar(body, cmov);
        vx_IrOp_addParam_s(cmov, VX_IR_NAME_COND, VX_IR_VALUE_BLK(cond));
        vx_IrOp_addParam_s(cmov, VX_IR_NAME_COND_THEN, VX_IR_VALUE_VAR(v0));
        vx_IrOp_addParam_s(cmov, VX_IR_NAME_COND_ELSE, VX_IR_VALUE_VAR(v1));
        vx_IrType* thenOutTy = vx_IrBlock_typeofVar(root, then->outs[0]);
        assert(thenOutTy);
        vx_IrOp_addOut(cmov, dest, thenOutTy);

        vx_IrBlock_addOut(body, dest);

        free(op->params);
        op->params = NULL;
        op->params_len = 0;

        op->id = VX_IR_OP_FLATTEN_PLEASE;

        vx_IrOp_addParam_s(op, VX_IR_NAME_BLOCK, VX_IR_VALUE_BLK(body));
    }
}
