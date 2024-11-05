#include <assert.h>
#include "../passes.h"

/**
 * @param outer assignment of var that is closest to conditional
 * @param outerOff the index of the assignment in outer
 * @param conditional block containing ther condtional assignment of var
 * @param condOff the index of the assignment in conditional
 * @param ifOp the if op that contains the conditional block
 * @param var affected variable
 * @param manipIn 
 */
static vx_IrVar megic(vx_IrBlock *outer,
                      vx_IrOp    *orig_assign,
                      vx_IrBlock *conditional,
                      vx_IrOp    *cond_op,
                      vx_IrOp    *ifOp,
                      const vx_IrVar    var,
                      const vx_OptIrVar manipIn)
{
    vx_IrType *type = NULL;
    for (size_t i = 0; i < orig_assign->outs_len; i ++) {
        if (orig_assign->outs[i].var == var) {
            type = orig_assign->outs[i].type;
            break;
        }
    }

    vx_IrBlock *then = vx_IrOp_param(ifOp, VX_IR_NAME_COND_THEN)->block;
    vx_IrValue *pels = vx_IrOp_param(ifOp, VX_IR_NAME_COND_ELSE);

    vx_IrBlock *els;
    if (pels == NULL) {
        els = vx_IrBlock_initHeap(then->parent, then->parent_op); // lazyness
        vx_IrOp_addParam_s(ifOp, VX_IR_NAME_COND_ELSE, VX_IR_VALUE_BLK(els));
        vx_IrBlock_addOut(els, var);
    } else {
        els = pels->block;
    }

    vx_IrVar thenVar = vx_IrBlock_newVar(then, NULL);
    vx_IrBlock_renameVar(then, var, thenVar, VX_RENAME_VAR_BOTH);

    vx_IrVar elseVar = vx_IrBlock_newVar(els, NULL);
    vx_IrBlock_renameVar(els, var, elseVar, VX_RENAME_VAR_BOTH);

    const vx_IrVar manipulate = vx_IrBlock_newVar(outer, ifOp);
    {
        vx_IrOp *oldstart = outer->first;
        outer->first = ifOp->next;
        vx_IrBlock_renameVar(outer, var, manipulate, VX_RENAME_VAR_BOTH);
        outer->first = oldstart;
    }

    vx_IrOp_addOut(ifOp, manipulate, type);
    vx_IrBlock_addOut(then, thenVar);
    vx_IrBlock_addOut(els, elseVar);

    return manipulate;
}

// TODO: I don't think we need manip 

// call megic somehow
// detect the patter from the inside out!!
vx_OptIrVar vx_CIrBlock_mksa_states(vx_CU* cu, vx_IrBlock *block)
{
    vx_IrBlock* root = vx_IrBlock_root(block); assert(root);
    vx_OptIrVar rvar = VX_IRVAR_OPT_NONE;

    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->id == VX_IR_OP_IF) {
            // inside out:
            FOR_PARAMS(op, MKARR(VX_IR_NAME_COND_THEN, VX_IR_NAME_COND_ELSE), param, {
                vx_IrBlock *conditional = param.block;
                vx_OptIrVar manip = vx_CIrBlock_mksa_states(cu, conditional);

                // are we assigning any variable directly in that block that we also assign on the outside before the inst?
                for (vx_IrOp *condAssignOp = conditional->first; condAssignOp; condAssignOp = condAssignOp->next) {
                    for (size_t l = 0; l < condAssignOp->outs_len; l ++) {
                        vx_IrVar var = condAssignOp->outs[l].var;

                        vx_IrOp *alwaysAssignOp = vx_IrBlock_vardeclOutBefore(block, var, op);
                        if (alwaysAssignOp == NULL)
                            continue;
                        rvar = VX_IRVAR_OPT_SOME(megic(alwaysAssignOp->parent, alwaysAssignOp, conditional, condAssignOp, op, var, manip));
                    }
                }
            });
            continue;
        }

        FOR_INPUTS(op, inp, {
            if (inp.type == VX_IR_VAL_BLOCK)
                (void) vx_CIrBlock_mksa_states(cu, inp.block);
        });
        
        if (op->id == VX_IR_OP_WHILE) {
            vx_IrBlock* cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;
            vx_IrBlock* body = vx_IrOp_param(op, VX_IR_NAME_LOOP_DO)->block;

            size_t declVars_len;
            vx_IrVar* declVars = vx_IrBlock_listDeclaredVarsRec(body, &declVars_len);

            for (size_t i = 0; i < declVars_len; i ++)
            {
                vx_IrVar var = declVars[i];

                if (vx_IrBlock_varDeclRec(body, var) != NULL && vx_IrBlock_vardeclOutBefore(block, var, op) != NULL)
                {
                    vx_IrType* ty = vx_IrBlock_typeofVar(block, var);

                    {
                    vx_IrVar nameInCond = vx_IrBlock_newVar(block, op);
                    vx_IrBlock_addIn(cond, nameInCond, ty);
                    vx_IrBlock_renameVar(cond, var, nameInCond, VX_RENAME_VAR_BOTH);
                    }

                    {
                    vx_IrVar nameInBody = vx_IrBlock_newVar(block, op);
                    vx_IrBlock_addIn(body, nameInBody, ty);
                    vx_IrBlock_renameVar(body, var, nameInBody, VX_RENAME_VAR_BOTH);
                    vx_IrBlock_addOut(body, nameInBody);
                    }

                    vx_IrOp_addArg(op, VX_IR_VALUE_VAR(var));
                    vx_IrOp_addOut(op, var, ty);
                }
            }
        }
    }

    return rvar;
}
