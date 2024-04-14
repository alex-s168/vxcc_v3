#include "ssa.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../utils.h"

bool ssablock_staticeval_var(const SsaBlock *block, const SsaVar var, SsaValue *dest) {
    const SsaOp *decl = ssablock_root(block)->as_root.vars[var].decl;
    if (decl == NULL)
        return false;

    if (decl->id == SSA_OP_IMM) {
        const SsaValue *value = ssaop_param(decl, "val");
        if (value == NULL)
            return false;

        if (value->type == SSA_VAL_VAR)
            return ssablock_staticeval_var(block, value->var, dest);

        *dest = *value;
        return true;
    }

    return false;
}

bool ssablock_mightbe_var(const SsaBlock *block, SsaVar var, SsaValue v) {
    SsaValue is;
    if (ssablock_staticeval_var(block, var, &is)) {
        return memcmp(&is, &v, sizeof(SsaValue)) == 0;
    }
    return true;
}

bool ssablock_alwaysis_var(const SsaBlock *block, SsaVar var, SsaValue v) {
    SsaValue is;
    if (!ssablock_staticeval_var(block, var, &is))
        return false;
    return memcmp(&is, &v, sizeof(SsaValue)) == 0;
}

void ssablock_staticeval(SsaBlock *block, SsaValue *v) {
    if (v->type == SSA_VAL_VAR)
        while (ssablock_staticeval_var(block, v->var, v)) {}
}