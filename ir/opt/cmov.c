#include "../opt.h"

static vx_IrVar block_res_as_var(vx_IrBlock *parent, vx_IrBlock *block) {
    vx_IrOp *op = vx_IrBlock_add_op_building(parent);
    vx_IrOp_init(op, VX_IR_OP_FLATTEN_PLEASE, parent);
    vx_IrVar dest = vx_IrBlock_new_var(parent, op);
    vx_IrOp_add_param_s(op, VX_IR_NAME_BLOCK, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = block });
    vx_IrOp_add_out(op, dest, vx_IrBlock_typeof_var(block, block->outs[0]));
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

        vx_IrBlock *then = pthen->block;
        if (vx_IrBlock_is_volatile(then))
            continue;

        vx_IrBlock *els = pelse->block;
        if (vx_IrBlock_is_volatile(els))
            continue;

        size_t total_cost = vx_IrBlock_inline_cost(then) + vx_IrBlock_inline_cost(els);
        if (total_cost > vx_g_optconfig.max_total_cmov_inline_cost)
            continue;

        vx_IrBlock *cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;


        vx_IrBlock *body = vx_IrBlock_init_heap(block, op);

        vx_IrVar v0 = block_res_as_var(body, then);
        vx_IrVar v1 = block_res_as_var(body, els);

        vx_IrOp *cmov = vx_IrBlock_add_op_building(body);
        vx_IrOp_init(cmov, VX_IR_OP_CMOV, body);
        vx_IrVar dest = vx_IrBlock_new_var(body, cmov);
        vx_IrOp_add_param_s(cmov, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = cond });
        vx_IrOp_add_param_s(cmov, VX_IR_NAME_COND_THEN, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = v0 });
        vx_IrOp_add_param_s(cmov, VX_IR_NAME_COND_ELSE, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = v1 });
        vx_IrOp_add_out(cmov, dest, vx_IrBlock_typeof_var(then, then->outs[0]));

        vx_IrBlock_add_out(body, dest);


        vx_IrTypedVar out = op->outs[0];

        free(op->params);
        op->params = NULL;
        op->params_len = 0;
        vx_IrOp_destroy(op);
        memset(op, 0, sizeof(vx_IrOp));
        vx_IrOp_init(op, VX_IR_OP_FLATTEN_PLEASE, block);

        vx_IrOp_add_param_s(op, VX_IR_NAME_BLOCK, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = body });
        vx_IrOp_add_out(op, out.var, out.type);
    }
}
