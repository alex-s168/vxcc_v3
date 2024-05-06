#include <assert.h>
#include <stdlib.h>

#include "ir.h"
#include "verify.h"

struct verify_vardecls_deeptraverse__data {
    size_t declcount;
    SsaVar var;
};

static void verify_vardecls_deeptraverse(SsaOp *op, void *dataIn) {
    struct verify_vardecls_deeptraverse__data *va = dataIn;

    for (size_t i = 0; i < op->outs_len; i ++)
        if (op->outs[i].var == va->var)
            va->declcount ++;

    for (size_t j = 0; j < op->params_len; j ++) {
        const SsaValue param = op->params[j].val;

        if (param.type == SSA_VAL_BLOCK) {
            for (size_t k = 0; k < param.block->ins_len; k ++)
                if (param.block->ins[k] == va->var)
                    va->declcount ++;
        }
    }
}

VerifyErrors irblock_verify(const SsaBlock *block, const OpPath path) {
    VerifyErrors errors;
    errors.len = 0;
    errors.items = NULL;

    irblock_verify_ssa_based(&errors, block, path);

    if (block->is_root) {
        for (size_t i = 0; i < block->as_root.vars_len; i ++) {
            if (block->as_root.vars[i].decl == NULL)
                continue;

            struct verify_vardecls_deeptraverse__data dat;
            dat.var = i;
            dat.declcount = 0;
            irview_deep_traverse(irview_of_all(block), verify_vardecls_deeptraverse, &dat);

            assert(dat.declcount > 0 /* WE REMOVED VARIABLE DECLARATION WITHOUT REMOVING IT FROM DB!!! */);

            if (dat.declcount > 1) {
                static char buf[256];
                sprintf(buf, "Variable %%%zu is assigned more than once!", i);
                VerifyError error = {
                    .path = (OpPath) { .ids = NULL, .len = 0 },
                    .error = "Variable assigned more than once",
                    .additional = buf
                };
                verifyerrors_add(&errors, &error);
            }
        }
    }

    return errors;
}
