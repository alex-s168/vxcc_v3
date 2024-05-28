#include <assert.h>

#include "../llir.h"

static void lower_into(vx_IrOp *oldops, size_t oldops_len, vx_IrBlock *dest);

static void into(vx_IrBlock *src, vx_IrOp *parent, vx_IrBlock *dest) {
    bool *usedarr = malloc(sizeof(bool) * parent->outs_len);
    for (size_t outid = 0; outid < parent->outs_len; outid ++) {
        vx_IrTypedVar var = parent->outs[outid];
        vx_IrVar vv = src->outs[outid];

        bool used = false;

        vx_IrOp *decl = vx_IrBlock_root_get_var_decl(vx_IrBlock_root(src->parent), vv);
        if (decl->parent != src)
            used = true;

        if (!used) {
            for (size_t opid = (decl - decl->parent->ops) + 1; opid < src->parent->ops_len; opid ++) {
                vx_IrOp *op = &src->parent->ops[opid];
                for (size_t i = 0; i < op->params_len; i ++) {
                    if (op->params[i].val.type == VX_IR_VAL_VAR && op->params[i].val.var == vv) {
                        used = true;
                        break;
                    }
                    if (op->params[i].val.type == VX_IR_VAL_BLOCK && vx_IrBlock_var_used(op->params[i].val.block, vv)) {
                        used = true;
                        break;
                    }
                }

                if (used)
                    break;
            }
        }

        usedarr[outid] = used;

        if (!used) {
            for (size_t decl_outid = 0; decl_outid < decl->outs_len; decl_outid ++) {
                if (decl->outs[decl_outid].var == vv) {
                    decl->outs[decl_outid].var = var.var;
                    break;
                }
            }
        }
    }
    
    lower_into(src->ops, src->ops_len, dest);
    
    for (size_t outid = 0; outid < parent->outs_len; outid ++) {
        if (usedarr[outid]) {
            vx_IrTypedVar var = parent->outs[outid];
            vx_IrVar vv = src->outs[outid];

            // emit mov 

            vx_IrOp init;
            vx_IrOp_init(&init, VX_IR_OP_IMM, dest);
            vx_IrOp_add_param_s(&init, VX_IR_NAME_VALUE, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = vv });
            vx_IrOp_add_out(&init, var.var, var.type);

            vx_IrBlock_add_op(dest, &init);
        }
    }

    free(usedarr);
}

static void lower_into(vx_IrOp *oldops, size_t oldops_len, vx_IrBlock *dest) {
    for (size_t i = 0; i < oldops_len; i ++) {
        vx_IrOp * op = &oldops[i];

        if (op->id == VX_IR_OP_IF) {
            vx_IrBlock *cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;
            
            vx_IrBlock *then = NULL; {
                vx_IrValue *val = vx_IrOp_param(op, VX_IR_NAME_COND_THEN);
                if (val)
                    then = val->block;
            }

            vx_IrBlock *els = NULL; {
                vx_IrValue *val = vx_IrOp_param(op, VX_IR_NAME_COND_ELSE);
                if (val)
                    els = val->block;
            }


            if (els == NULL && then == NULL) {
                continue;
            }

            vx_IrVar cond_var = cond->outs[0];

            lower_into(cond->ops, cond->ops_len, dest);

            if (els && then) {
                //   cond .then COND
                //   ELSE
                //   goto .end
                // .then:
                //   THEN
                // .end:

                size_t jmp_cond_idx;
                {
                    vx_IrOp *jmp_cond = vx_IrBlock_add_op_building(dest);
                    jmp_cond_idx = jmp_cond - dest->ops;
                    vx_IrOp_init(jmp_cond, VX_LIR_COND, dest);
                    vx_IrOp_add_param_s(jmp_cond, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = cond_var });
                }

                into(els, op, dest);

                size_t jmp_end_idx;
                {
                    vx_IrOp *jmp_end = vx_IrBlock_add_op_building(dest);
                    jmp_end_idx = jmp_end - dest->ops;
                    vx_IrOp_init(jmp_end, VX_LIR_GOTO, dest);
                }

                size_t label_then = vx_IrBlock_insert_label_op(dest);

                {
                    vx_IrOp *jmp_cond = dest->ops + jmp_cond_idx;
                    vx_IrOp_add_param_s(jmp_cond, VX_IR_NAME_ID, (vx_IrValue) { .type = VX_IR_VAL_ID, .id = label_then });
                }

                into(then, op, dest);
                size_t label_end = vx_IrBlock_insert_label_op(dest);

                {
                    vx_IrOp *jmp_end = dest->ops + jmp_end_idx;
                    vx_IrOp_add_param_s(jmp_end, VX_IR_NAME_ID, (vx_IrValue) { .type = VX_IR_VAL_ID, .id = label_end });
                }
            } else {
                if (then) {
                    assert(!els);

                    // we only have a then block

                    // else <= then  and cond_var = !cond_var
                    
                    // TODO: move invert into higher level transform (because of constant eval and op opt)

                    vx_IrOp *inv = vx_IrBlock_add_op_building(dest);
                    vx_IrVar new = vx_IrBlock_new_var(dest, inv);
                    vx_IrOp_init(inv, VX_IR_OP_NOT, dest);
                    vx_IrOp_add_out(inv, new, vx_IrBlock_typeof_var(cond, cond_var));
                    vx_IrOp_add_param_s(inv, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = cond_var });
                    cond_var = new;

                    els = then;
                    then = NULL;
                }

                assert(!then);
                assert(els);

                // conditional jump to after the else block 

                size_t jump_idx;
                {
                    vx_IrOp *jump = vx_IrBlock_add_op_building(dest);
                    jump_idx = jump - dest->ops;
                    vx_IrOp_init(jump, VX_LIR_COND, dest);
                    vx_IrOp_add_param_s(jump, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = cond_var });
                }

                into(els, op, dest);

                size_t label_id = vx_IrBlock_insert_label_op(dest);

                {
                    vx_IrOp *jump = dest->ops + jump_idx;
                    vx_IrOp_add_param_s(jump, VX_IR_NAME_ID, (vx_IrValue) { .type = VX_IR_VAL_ID, .id = label_id });
                }
            }
        }
        else if (op->id == VX_IR_OP_FLATTEN_PLEASE) {
            vx_IrBlock *body = vx_IrOp_param(op, VX_IR_NAME_BLOCK)->block;
            into(body, op, dest);    
        }
        else if (op->id == VX_IR_OP_CMOV) {
            vx_IrValue *pcond = vx_IrOp_param(op, VX_IR_NAME_COND);
            vx_IrBlock *cond = pcond->block;

            vx_IrVar cond_var = cond->outs[0];

            lower_into(cond->ops, cond->ops_len, dest);

            pcond->type = VX_IR_VAL_VAR;
            pcond->var = cond_var;

            vx_IrBlock_add_op(dest, op);
        }
        // TODO: lower loops
        else {
            // TODO: lower block params
            vx_IrBlock_add_op(dest, op); 
        }
    }
}

void vx_IrBlock_llir_lower(vx_IrBlock *block) {
    vx_IrOp *old_ops = block->ops;
    block->ops = NULL;
    const size_t old_ops_len = block->ops_len;
    block->ops_len = 0;
    lower_into(old_ops, old_ops_len, block);
    free(old_ops);
}
