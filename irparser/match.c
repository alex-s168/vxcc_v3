#include "parser.h"

static bool try_match_op(uint32_t i, OptPatternMatch* out, CompOperation* op, vx_IrOp* irop, bool *placeholders_matched)
{
    if (op->specific.op_type != irop->id)
        return false;
    if (op->specific.outputs.count != irop->outs_len)
        return false;

    for (uint32_t i = 0; i < op->specific.operands.count; i ++)
    {
        CompOperand* operand = &op->specific.operands.items[i];

        vx_IrValue* val = vx_IrOp_param(irop, operand->name);
        if (!val) return false;

        switch (operand->type)
        {
            case OPERAND_TYPE_PLACEHOLDER: {
                if (val->type != VX_IR_VAL_VAR) return false;
                uint32_t ph = operand->v.placeholder;
                if (placeholders_matched[ph]) {
                    if (out->matched_placeholders[ph] != val->var)
                        return false; 
                } else {
                    out->matched_placeholders[ph] = val->var;
                    placeholders_matched[ph] = true;
                }
            } break;

            case OPERAND_TYPE_IMM_INT: {
                if (val->type != VX_IR_VAL_IMM_INT) return false;
                if (val->imm_int != operand->v.imm_int) return false;
            } break; 

            case OPERAND_TYPE_IMM_FLT: {
                if (val->type != VX_IR_VAL_IMM_FLT) return false;
                if (val->imm_flt != operand->v.imm_flt) return false;
            } break;
        }
    }

    return true;
}

static bool try_match_at(OptPatternMatch* out, CompPattern pattern, vx_IrOp* irop, bool *placeholders_matched)
{
    for (uint32_t i = 0; i < pattern.count; i ++) {
        CompOperation* current = &pattern.items[i];
        CompOperation* next = i + 1 < pattern.count ? &pattern.items[i + 1] : NULL;

        if (current->is_any && next && next->is_any) continue;

        if (current->is_any && next) {
            vx_IrOp* mfirst = irop;
            vx_IrOp* mlast = NULL;
            while (!try_match_op(i + 1, out, next, irop, placeholders_matched)) {
                mlast = irop;
                irop = irop->next;
            }
            if (irop == mfirst) mfirst = NULL;
            out->matched_instrs[i].first = mfirst;
            out->matched_instrs[i].last = mlast; 
            i ++; // skip processing of next, since already matched
            continue;
        }
        else if (current->is_any) {
            out->matched_instrs[i].first = irop;
            for (; irop; irop = irop->next);
            out->matched_instrs[i].last = irop;
            break; // no next
        }
        else {
            if (!try_match_op(i, out, current, irop, placeholders_matched))
                return false;
            irop = irop->next;
        }
    }

    return true;
}

OptPatternMatch Pattern_matchNext(vx_IrOp* first, CompPattern pattern)
{
    OptPatternMatch match;
    match.found = false;
    match.last = NULL;

    match.matched_placeholders = malloc(pattern.placeholders_count * sizeof(vx_IrVar));
    match.matched_instrs = malloc(pattern.count * sizeof(PatternInstMatch));
    bool *placeholders_matched = calloc(pattern.placeholders_count, sizeof(bool));

    for (; first; first = first->next)
        if (try_match_at(&match, pattern, first, placeholders_matched))
            break;

    free(placeholders_matched);

    return match;
}

void OptPatternMatch_free(OptPatternMatch match)
{
    free(match.matched_placeholders);
    free(match.matched_instrs);
}
