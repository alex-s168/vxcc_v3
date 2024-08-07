#include <assert.h>

#include "../ir.h"

/**
 * megic
 *
 * example input:
 * \code
 * %0 = important()
 * important()
 * if (%1) {
 *     %0 = 23
 * }
 * \endcode
 *
 * stage 1:
 * \code
 * %0 = important()
 * %2 = %0
 * if (%1) {
 *     %2 = 23
 * }
 * \endcode
 * The reason for this is that we cannot move the assignment of %0 arround because it has side effects
 *
 * stage 2:
 * we don't have an example for this so here is an explanation:
 * when the variable gets set multiple times in the conditional block,
 * we need to do the same thing as in \see irblock_mksa_final but capture the last assignment and operate with that.
 *
 * stage 3:
 * \code
 * %0 = important()
 * %2 = if (%1) {
 *     23
 * } else {
 *     %0
 * }
 * \endcode
 *
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
    // stage 1
    const vx_IrVar manipulate = vx_IrBlock_new_var(outer, ifOp);
    {
        vx_IrOp *oldstart = outer->first;
        outer->first = orig_assign->next;
        vx_IrBlock_rename_var(outer, var, manipulate);
        outer->first = oldstart;
    }

    // stage 2
    vx_IrVar last_cond_assign = manipulate;
    for (vx_IrOp *op = cond_op; op; op = op->next) {
        // make sure that we assign in this op
        bool found = false;
        for (size_t j = 0; j < op->outs_len; j ++) {
            if (op->outs[j].var == last_cond_assign) {
                found = true;
                break;
            }
        }
        if (!found)
            continue;

        // we don't have to check children because megic gets called from the inside out

        const vx_IrVar new = vx_IrBlock_new_var(outer, op);
        // we include the current index on purpose
        vx_IrOp *oldstart = conditional->first;
        conditional->first = op;
        vx_IrBlock_rename_var(conditional, last_cond_assign, new);
        last_cond_assign = new;
        conditional->first = oldstart;
    }

    // stage 3

    // for people that can't keep track of shit, like me, here is a short summary of the current state:
    //  var              <=> the unconditional var that we need to return in the else block
    //  last_cond_assign <=> the conditional var that we need to return in the then block
    //  manipulate       <=> the new target var

    vx_IrType *type = NULL;
    for (size_t i = 0; i < orig_assign->outs_len; i ++)
        if (orig_assign->outs[i].var == var)
            type = orig_assign->outs[i].type;

    vx_IrBlock *then = vx_IrOp_param(ifOp, VX_IR_NAME_COND_THEN)->block;
    const vx_IrValue *pels = vx_IrOp_param(ifOp, VX_IR_NAME_COND_ELSE);

    vx_IrBlock *els;
    if (pels == NULL) {
        els = vx_IrBlock_init_heap(then->parent, then->parent_op); // lazyness
        vx_IrOp_add_param_s(ifOp, VX_IR_NAME_COND_ELSE, (vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = els });
    } else {
        els = pels->block;
    }

    if (conditional == els) {
        vx_IrBlock *temp = then;
        then = els;
        els = temp;
    }

    vx_IrOp_add_out(ifOp, manipulate, type);
    if (manipIn.present)
        vx_IrBlock_add_out(then, manipIn.var);
    else
        vx_IrBlock_add_out(then, last_cond_assign);
    vx_IrBlock_add_out(els, var);

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

            // TODO: make work if we have a else block and assign there too!!!!

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
