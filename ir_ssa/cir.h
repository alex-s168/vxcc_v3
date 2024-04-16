#ifndef CIR_H
#define CIR_H

#include "ssa.h"

/** transform cfor to c ir primitives that are representable in ssa ir */
void cirblock_normalize(SsaBlock *block);

void cirblock_mksa_states(SsaBlock *block);

void cirblock_mksa_final(SsaBlock *block);

VerifyErrors cirblock_verify(const SsaBlock *block, OpPath path);

static int cir_verify(const SsaBlock *block) {
    OpPath path;
    path.ids = NULL;
    path.len = 0;
    const VerifyErrors errs = cirblock_verify(block, path);
    verifyerrors_print(errs, stderr);
    verifyerrors_free(errs);
    return errs.len > 0;
}

#endif //CIR_H
