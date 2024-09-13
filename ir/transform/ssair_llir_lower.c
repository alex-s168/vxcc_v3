#include <assert.h>

#include "../llir.h"

static void lower_into(vx_IrBlock *old, vx_IrBlock *dest);

static void into(vx_IrBlock *src, vx_IrOp *parent, vx_IrBlock *dest) {
    bool *usedarr = malloc(sizeof(bool) * parent->outs_len);
    assert(usedarr);
    for (size_t outid = 0; outid < parent->outs_len; outid ++) {
        vx_IrTypedVar var = parent->outs[outid];
        vx_IrVar vv = src->outs[outid];

        bool used = false;

        vx_IrOp *decl = vx_IrBlock_root(src->parent)->as_root.vars[vv].decl;
        if (decl->parent != src)
            used = true;

        if (!used) {
            for (vx_IrOp *op = decl->next; op; op = op->next) {
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
    
    lower_into(src, dest);

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

static void lower_into(vx_IrBlock *old, vx_IrBlock *dest) {
    for (vx_IrOp *op = old->first; op; op = op->next) {
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

            lower_into(cond, dest);

            if (els && then) {
                //   cond .then COND
                //   ELSE
                //   goto .end
                // .then:
                //   THEN
                // .end:

                vx_IrOp *jmp_cond = vx_IrBlock_add_op_building(dest);
                vx_IrOp_init(jmp_cond, VX_LIR_OP_COND, dest);
                vx_IrOp_add_param_s(jmp_cond, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = cond_var });

                into(els, op, dest);

                vx_IrOp *jmp_end = vx_IrBlock_add_op_building(dest);
                vx_IrOp_init(jmp_end, VX_LIR_OP_GOTO, dest);

                size_t label_then = vx_IrBlock_append_label_op(dest);

                vx_IrOp_add_param_s(jmp_cond, VX_IR_NAME_ID, (vx_IrValue) { .type = VX_IR_VAL_ID, .id = label_then });

                into(then, op, dest);

                size_t label_end = vx_IrBlock_append_label_op(dest);

                vx_IrOp_add_param_s(jmp_end, VX_IR_NAME_ID, (vx_IrValue) { .type = VX_IR_VAL_ID, .id = label_end });
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

                vx_IrOp *jump = vx_IrBlock_add_op_building(dest);
                vx_IrOp_init(jump, VX_LIR_OP_COND, dest);
                vx_IrOp_add_param_s(jump, VX_IR_NAME_COND, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = cond_var });

                into(els, op, dest);

                size_t label_id = vx_IrBlock_append_label_op(dest);

                vx_IrOp_add_param_s(jump, VX_IR_NAME_ID, (vx_IrValue) { .type = VX_IR_VAL_ID, .id = label_id });
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

            lower_into(cond, dest);

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

    if (old->is_root) {
        vx_IrOp* ret = vx_IrBlock_add_op_building(dest);
        vx_IrOp_init(ret, VX_IR_OP_RETURN, dest);
        for (size_t i = 0; i < old->outs_len; i++) {
            vx_IrOp_add_arg(ret, (vx_IrValue) { .type = VX_IR_VAL_VAR, .var = old->outs[i] });
        }

        dest->ll_out_types = malloc(sizeof(vx_IrType*) * old->outs_len);
        dest->ll_out_types_len = old->outs_len;
        for (size_t i = 0; i < old->outs_len; i ++)
             dest->ll_out_types[i] = vx_IrBlock_typeof_var(old, old->outs[i]);

        free(dest->outs);
        dest->outs = NULL;
        dest->outs_len = 0;
    }
}

void vx_IrBlock_llir_lower(vx_IrBlock *block) {
    static vx_IrBlock copy;
    copy = *block;
    block->first = NULL;
    lower_into(&copy, block);
}
