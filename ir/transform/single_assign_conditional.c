#include <assert.h>

#include "../ir.h"
#include "../cir.h"

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

    const vx_IrVar manipulate = vx_IrBlock_new_var(outer, ifOp);
    {
        vx_IrOp *oldstart = outer->first;
        outer->first = orig_assign->next;
        vx_IrBlock_rename_var(outer, var, manipulate);
        outer->first = oldstart;
    }

    vx_IrBlock *then = vx_IrOp_param(ifOp, VX_IR_NAME_COND_THEN)->block;
    vx_IrValue *pels = vx_IrOp_param(ifOp, VX_IR_NAME_COND_ELSE);

    vx_IrBlock *els;
    if (pels == NULL) {
        els = vx_IrBlock_init_heap(then->parent, then->parent_op); // lazyness
        vx_IrOp_add_param_s(ifOp, VX_IR_NAME_COND_ELSE, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = els });
    } else {
        els = pels->block;
    }

    vx_IrVar thenVar = vx_IrBlock_new_var(then, NULL);
    vx_IrBlock_rename_var(then, var, thenVar);

    vx_IrVar elseVar = vx_IrBlock_new_var(els, NULL);
    vx_IrBlock_rename_var(els, var, elseVar);

    thenVar = vx_CIrBlock_partial_mksaFinal_norec(then, thenVar);
    elseVar = vx_CIrBlock_partial_mksaFinal_norec(els, elseVar);
        
    vx_IrOp_add_out(ifOp, manipulate, type);
    vx_IrBlock_add_out(then, thenVar);
    vx_IrBlock_add_out(els, elseVar);

    return manipulate;
}

// call megic somehow
// detect the patter from the inside out!!
vx_OptIrVar vx_CIrBlock_mksa_states(vx_IrBlock *block)
{
    vx_OptIrVar rvar = VX_IRVAR_OPT_NONE;

    for (vx_IrOp *ifOp = block->first; ifOp; ifOp = ifOp->next) {
        if (ifOp->id != VX_IR_OP_IF)
            continue;

        // inside out:

        FOR_PARAMS(ifOp, MKARR(VX_IR_NAME_COND_THEN, VX_IR_NAME_COND_ELSE), param, {
            vx_IrBlock *conditional = param.block;
            vx_OptIrVar manip = vx_CIrBlock_mksa_states(conditional);

            // are we assigning any variable directly in that block that we also assign on the outside before the inst?
            for (vx_IrOp *condAssignOp = conditional->first; condAssignOp; condAssignOp = condAssignOp->next) {
                for (size_t l = 0; l < condAssignOp->outs_len; l ++) {
                    vx_IrVar var = condAssignOp->outs[l].var;

                    vx_IrOp *alwaysAssignOp = vx_IrBlock_vardecl_out_before(block, var, ifOp);
                    if (alwaysAssignOp == NULL)
                        continue;
                    rvar = VX_IRVAR_OPT_SOME(megic(alwaysAssignOp->parent, alwaysAssignOp, conditional, condAssignOp, ifOp, var, manip));
                }
            }
        });
    }

    return rvar;
}
