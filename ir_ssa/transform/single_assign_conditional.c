#include <assert.h>

#include "../ssa.h"

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
 * we need to do the same thing as in \see ssablock_mksa_final but capture the last assignment and operate with that.
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
 */
static void megic(SsaBlock *outer, const size_t outerOff, SsaBlock *conditional, const size_t condOff, SsaOp *ifOp, const SsaVar var) {
    // stage 1
    const SsaVar manipulate = ssablock_new_var(outer, ifOp);
    {
        SsaView rename = ssaview_of_all(outer);
        rename.start = outerOff + 1;
        ssaview_rename_var(rename, outer, var, manipulate);
    }
        
    // stage 2
    SsaVar last_cond_assign = manipulate;
    for (size_t i = condOff; i < conditional->ops_len; i ++) {
        SsaOp *op = &conditional->ops[i];

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

        const SsaVar new = ssablock_new_var(outer, op);
        // we include the current index on purpose
        SsaView rename = ssaview_of_all(conditional);
        rename.start = i;
        ssaview_rename_var(rename, conditional, last_cond_assign, new);
        last_cond_assign = new;
    }

    // stage 3

    // for people that can't keep track of shit, like me, here is a short summary of the current state:
    // var <=> the unconditional var that we need to return in the else block
    // last_cond_assign <=> the conditional var that we need to return in the then block
    // manipulate <=> the new target var

    const SsaOp *orig_assign = &outer->ops[outerOff];
    SsaType type = NULL;
    for (size_t i = 0; i < orig_assign->outs_len; i ++)
        if (orig_assign->outs[i].var == var)
            type = orig_assign->outs[i].type;

    SsaBlock *then = ssaop_param(ifOp, SSA_NAME_COND_THEN)->block;
    const SsaValue *pels = ssaop_param(ifOp, SSA_NAME_COND_ELSE);

    SsaBlock *els;
    if (pels == NULL) {
        els = ssablock_heapalloc(then->parent, then->parent_index); // lazyness
        ssaop_add_param_s(ifOp, SSA_NAME_COND_ELSE, (SsaValue) { .type = SSA_VAL_BLOCK, .block = els });
    } else {
        els = pels->block;
    }

    if (conditional == els) {
        SsaBlock *temp = then;
        then = els;
        els = temp;
    }

    ssaop_add_out(ifOp, manipulate, type);
    ssablock_add_out(then, last_cond_assign);
    ssablock_add_out(els, var);
}

// call megic somehow
// detect the patter from the inside out!!
void cirblock_mksa_states(SsaBlock *block) {
    for (size_t i = 0; i < block->ops_len; i ++) {
        SsaOp *ifOp = &block->ops[i];
        if (ifOp->id != SSA_OP_IF)
            continue;

        // inside out:
        assert(ifOp->params_len >= 2);
        assert(ifOp->params_len <= 3);
        for (size_t j = 0; j < ifOp->params_len; j ++) {
            if (ifOp->params[j].val.type != SSA_VAL_BLOCK)
                continue;

            SsaBlock *conditional = ifOp->params[j].val.block;
            cirblock_mksa_states(conditional);

            // TODO: make work if we have a else block and assign there too!!!!

            // are we assigning any variable directly in that block that we also assign on the outside before the inst?
            for (size_t k = 0; k < conditional->ops_len; k ++) {
                const SsaOp *condAssignOp = &conditional->ops[k];
                for (size_t l = 0; l < condAssignOp->outs_len; l ++) {
                    const SsaVar var = condAssignOp->outs[l].var;

                    const SsaOp *alwaysAssignOp = ssablock_inside_out_vardecl_before(block, var, i);
                    if (alwaysAssignOp == NULL)
                        continue;

                    megic(alwaysAssignOp->parent, alwaysAssignOp->parent->ops - alwaysAssignOp, conditional, k, ifOp, var);
                }
            }
        }
    }
}
