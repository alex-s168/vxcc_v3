#include <assert.h>

#include "../passes.h"

void vx_IrBlock_llir_preLower_ifs(vx_CU* cu, vx_IrBlock *block)
{
    for (vx_IrOp* op = block->first; op; op = op->next)
    {
        FOR_INPUTS(op, inp, {
            if (inp.type == VX_IR_VAL_BLOCK)
                vx_IrBlock_llir_preLower_ifs(cu, inp.block);
        });

        if (op->id != VX_IR_OP_IF)
            continue;

        vx_IrValue* cond = vx_IrOp_param(op, VX_IR_NAME_COND);

        vx_IrBlock* condbl = cond->block;
        *cond = VX_IR_VALUE_VAR(condbl->outs[0]);

        if (condbl->first) {
            for (vx_IrOp* i = condbl->first; i; i = i->next)
                vx_IrOp_updateParent(i, block);

            vx_IrOp* pred = vx_IrOp_predecessor(op);
            if (pred)
                pred->next = condbl->first;
            else 
                block->first = condbl->first;
            vx_IrBlock_tail(condbl)->next = op;
        }
    }
}

static void lower_into(vx_IrBlock *old, vx_IrBlock *dest, vx_IrBlock* newParent, size_t continueLabel, size_t breakLabel, vx_IrOp* loopOP);

static void into(vx_IrBlock *src, vx_IrOp *parent, vx_IrBlock *dest, size_t continueLabel, size_t breakLabel, vx_IrOp* loopOP) {
    bool *usedarr = malloc(sizeof(bool) * parent->outs_len);
    assert(usedarr);
    for (size_t outid = 0; outid < parent->outs_len; outid ++) {
        vx_IrTypedVar var = parent->outs[outid];
        vx_IrVar vv = src->outs[outid];

        bool used = false;

        vx_IrOp *decl = vx_IrBlock_root(src->parent)->as_root.vars[vv].decl;

        if (decl == NULL) // probably! arg
            used = true;
        else if (decl->parent != src)
            used = true;

        if (!used) {
            for (vx_IrOp *op = decl->next; op; op = op->next) {
                for (size_t i = 0; i < op->params_len; i ++) {
                    if (op->params[i].val.type == VX_IR_VAL_VAR && op->params[i].val.var == vv) {
                        used = true;
                        break;
                    }
                    if (op->params[i].val.type == VX_IR_VAL_BLOCK && vx_IrBlock_varUsed(op->params[i].val.block, vv)) {
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
    
    lower_into(src, dest, dest, continueLabel, breakLabel, loopOP);

    for (size_t outid = 0; outid < parent->outs_len; outid ++) {
        if (usedarr[outid]) {
            vx_IrTypedVar var = parent->outs[outid];
            vx_IrVar vv = src->outs[outid];

            // emit mov 

            vx_IrOp init;
            vx_IrOp_init(&init, VX_IR_OP_IMM, dest);
            vx_IrOp_addParam_s(&init, VX_IR_NAME_VALUE, VX_IR_VALUE_VAR(vv));
            vx_IrOp_addOut(&init, var.var, var.type);

            vx_IrBlock_addOp(dest, &init);
        }
    }

    free(usedarr);
}

static void lower_into(vx_IrBlock *old, vx_IrBlock *dest, vx_IrBlock *newParent, size_t continueLabel, size_t breakLabel, vx_IrOp* loopOP) {
    for (vx_IrOp *op = old->first; op; op = op->next) {
        op->parent = newParent;

        FOR_INPUTS_REF(op, inp, {
            if (inp->type == VX_IR_VAL_UNINIT)
                *inp = VX_IR_VALUE_IMM_INT(69); // we can do whatever we want to do lol
        });

        if (op->id == VX_IR_OP_IF) {
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

            vx_IrVar cond_var = vx_IrOp_param(op, VX_IR_NAME_COND)->var;
            vx_IrType* cond_var_ty = vx_IrBlock_typeofVar(op->parent, cond_var);

            // lower_into(cond, dest, newParent, continueLabel, breakLabel, loopOP);

            if (els && then) {
                //   cond .then COND
                //   ELSE
                //   goto .end
                // .then:
                //   THEN
                // .end:

                vx_IrOp *jmp_cond = vx_IrBlock_addOpBuilding(dest);
                vx_IrOp_init(jmp_cond, VX_IR_OP_COND, dest);
                vx_IrOp_addParam_s(jmp_cond, VX_IR_NAME_COND, VX_IR_VALUE_VAR(cond_var));

                into(els, op, dest, continueLabel, breakLabel, loopOP);

                vx_IrOp *jmp_end = vx_IrBlock_addOpBuilding(dest);
                vx_IrOp_init(jmp_end, VX_IR_OP_GOTO, dest);

                size_t label_then = vx_IrBlock_appendLabelOp(dest);

                vx_IrOp_addParam_s(jmp_cond, VX_IR_NAME_ID, VX_IR_VALUE_ID(label_then));

                into(then, op, dest, continueLabel, breakLabel, loopOP);

                size_t label_end = vx_IrBlock_appendLabelOp(dest);

                vx_IrOp_addParam_s(jmp_end, VX_IR_NAME_ID, VX_IR_VALUE_ID(label_end));
            } else {
                if (then) {
                    assert(!els);

                    // we only have a then block

                    // else <= then  and cond_var = !cond_var
                    
                    // TODO: move invert into higher level transform (because of constant eval and op opt)

                    vx_IrOp *inv = vx_IrBlock_addOpBuilding(dest);
                    vx_IrVar new = vx_IrBlock_newVar(dest, inv);
					vx_IrBlock_markVarOrigin(dest, cond_var, new);
                    vx_IrOp_init(inv, VX_IR_OP_NOT, dest);
                    vx_IrOp_addOut(inv, new, cond_var_ty);
                    vx_IrOp_addParam_s(inv, VX_IR_NAME_COND, VX_IR_VALUE_VAR(cond_var));
                    cond_var = new;

                    els = then;
                    then = NULL;
                }

                assert(!then);
                assert(els);

                // conditional jump to after the else block 

                vx_IrOp *jump = vx_IrBlock_addOpBuilding(dest);
                vx_IrOp_init(jump, VX_IR_OP_COND, dest);
                vx_IrOp_addParam_s(jump, VX_IR_NAME_COND, VX_IR_VALUE_VAR(cond_var));

                into(els, op, dest, continueLabel, breakLabel, loopOP);

                size_t label_id = vx_IrBlock_appendLabelOp(dest);

                vx_IrOp_addParam_s(jump, VX_IR_NAME_ID, VX_IR_VALUE_ID(label_id));
            }
        }
        else if (op->id == VX_IR_OP_CMOV) {

            vx_IrValue *pcond = vx_IrOp_param(op, VX_IR_NAME_COND);
            vx_IrBlock *cond = pcond->block;

            vx_IrVar cond_var = cond->outs[0];

            lower_into(cond, dest, newParent, continueLabel, breakLabel, loopOP);

            pcond->type = VX_IR_VAL_VAR;
            pcond->var = cond_var;

            vx_IrBlock_addOp(dest, op);
        }
        else if (op->id == VX_IR_OP_INFINITE) {

            // states are stored in op->outs

            vx_IrBlock* body = vx_IrOp_param(op, VX_IR_NAME_LOOP_DO)->block;

            for (size_t i = 0; i < op->outs_len; i ++) {
                vx_IrOp* mov = vx_IrBlock_addOpBuilding(dest);
                vx_IrOp_init(mov, VX_IR_OP_IMM, dest);
                vx_IrOp_addOut(mov, op->outs[i].var, op->outs[i].type);
                vx_IrOp_addParam_s(mov, VX_IR_NAME_VALUE, op->args[i]);
            }

            for (size_t i = 0; i < op->outs_len; i ++) {
                vx_IrBlock_renameVar(body, body->ins[i].var, op->outs[i].var, VX_RENAME_VAR_BOTH);
            }

            size_t newContinueLabel = vx_IrBlock_appendLabelOp(dest);
            size_t newBreakLabel = vx_IrBlock_newLabel(dest, NULL);

            lower_into(body, dest, newParent, newContinueLabel, newBreakLabel, op);

            for (size_t i = 0; i < op->outs_len; i ++) {
				vx_IrBlock_renameVar(dest, body->outs[i], op->outs[i].var, VX_RENAME_VAR_BOTH);
            }

            vx_IrOp* jmp = vx_IrBlock_addOpBuilding(dest);
            vx_IrOp_init(jmp, VX_IR_OP_GOTO, dest);
            vx_IrOp_addParam_s(jmp, VX_IR_NAME_ID, VX_IR_VALUE_ID(newContinueLabel));

            vx_IrBlock_appendLabelOpPredefined(dest, newBreakLabel);

        }
        else if (op->id == VX_IR_OP_CONTINUE) {

            assert(loopOP);

            for (size_t i = 0; i < old->outs_len; i ++) {
                vx_IrOp* mov = vx_IrBlock_addOpBuilding(dest);
                vx_IrOp_init(mov, VX_IR_OP_IMM, dest);
                vx_IrOp_addOut(mov, loopOP->outs[i].var, loopOP->outs[i].type);
                vx_IrOp_addParam_s(mov, VX_IR_NAME_VALUE, op->args[i]);
            }

            vx_IrOp* jmp = vx_IrBlock_addOpBuilding(dest);
            vx_IrOp_init(jmp, VX_IR_OP_GOTO, dest);
            vx_IrOp_addParam_s(jmp, VX_IR_NAME_ID, VX_IR_VALUE_ID(continueLabel));

        }
        else if (op->id == VX_IR_OP_BREAK) {

            assert(loopOP);

            for (size_t i = 0; i < old->outs_len; i ++) {
                vx_IrOp* mov = vx_IrBlock_addOpBuilding(dest);
                vx_IrOp_init(mov, VX_IR_OP_IMM, dest);
                vx_IrOp_addOut(mov, loopOP->outs[i].var, loopOP->outs[i].type);
                vx_IrOp_addParam_s(mov, VX_IR_NAME_VALUE, op->args[i]);
            }

            vx_IrOp* jmp = vx_IrBlock_addOpBuilding(dest);
            vx_IrOp_init(jmp, VX_IR_OP_GOTO, dest);
            vx_IrOp_addParam_s(jmp, VX_IR_NAME_ID, VX_IR_VALUE_ID(breakLabel));

        }
        else {
            // TODO: lower block params
            vx_IrBlock_addOp(dest, op); 
        }
    }

    if (old->is_root) {
        vx_IrOp* ret = vx_IrBlock_addOpBuilding(dest);
        vx_IrOp_init(ret, VX_IR_OP_RETURN, dest);
        for (size_t i = 0; i < old->outs_len; i++) {
            vx_IrOp_addArg(ret, VX_IR_VALUE_VAR(old->outs[i]));
        }

        dest->ll_out_types = malloc(sizeof(vx_IrType*) * old->outs_len);
        dest->ll_out_types_len = old->outs_len;
        for (size_t i = 0; i < old->outs_len; i ++)
             dest->ll_out_types[i] = vx_IrBlock_typeofVar(old, old->outs[i]);

        free(dest->outs);
        dest->outs = NULL;
        dest->outs_len = 0;
    }
}

void vx_IrBlock_llir_lower(vx_CU* cu, vx_IrBlock *block) {
    static vx_IrBlock copy;
    copy = *block;
    block->first = NULL;
    lower_into(&copy, block, block, 0, 0, NULL);

    for (vx_IrOp* op = block->first; op; op = op->next)
        op->parent = block;
}
