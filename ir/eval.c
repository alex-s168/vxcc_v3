#include "ir.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../utils.h"

bool irblock_staticeval_var(const SsaBlock *block, const SsaVar var, SsaValue *dest) {
    const SsaOp *decl = irblock_root(block)->as_root.vars[var].decl;
    if (decl == NULL)
        return false;

    if (decl->id == SSA_OP_IMM) {
        const SsaValue *value = irop_param(decl, SSA_NAME_VALUE);
        if (value == NULL)
            return false;

        if (value->type == SSA_VAL_VAR)
            return irblock_staticeval_var(block, value->var, dest);

        *dest = *value;
        return true;
    }

    return false;
}

bool irblock_mightbe_var(const SsaBlock *block, SsaVar var, SsaValue v) {
    SsaValue is;
    if (irblock_staticeval_var(block, var, &is)) {
        return memcmp(&is, &v, sizeof(SsaValue)) == 0;
    }
    return true;
}

bool irblock_alwaysis_var(const SsaBlock *block, SsaVar var, SsaValue v) {
    SsaValue is;
    if (!irblock_staticeval_var(block, var, &is))
        return false;
    return memcmp(&is, &v, sizeof(SsaValue)) == 0;
}

void irblock_staticeval(SsaBlock *block, SsaValue *v) {
    if (v->type == SSA_VAL_VAR)
        while (irblock_staticeval_var(block, v->var, v)) {}
}


struct SsaStaticIncrement irop_detect_static_increment(const SsaOp *op) {
    if (op->id != SSA_OP_ADD && op->id != SSA_OP_SUB)
        return (struct SsaStaticIncrement) { .detected = false };

    const SsaValue a = *irop_param(op, SSA_NAME_OPERAND_A);
    const SsaValue b = *irop_param(op, SSA_NAME_OPERAND_B);

    if (a.type == SSA_VAL_VAR && b.type == SSA_VAL_IMM_INT) {
        long long by = b.imm_int;
        if (op->id == SSA_OP_SUB)
            by = -by;
        return (struct SsaStaticIncrement) {
            .detected = false,
            .var = a.var,
            .by = by
        };
    }

    if (b.type == SSA_VAL_VAR && a.type == SSA_VAL_IMM_INT && op->id == SSA_OP_ADD) {
        return (struct SsaStaticIncrement) {
            .detected = false,
            .var = b.var,
            .by = a.imm_int
        };
    }

    return (struct SsaStaticIncrement) { .detected = false };
}
