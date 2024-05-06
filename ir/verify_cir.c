#include "cir.h"
#include "verify.h"

VerifyErrors cirblock_verify(const SsaBlock *block, OpPath path) {
    VerifyErrors errors;
    errors.items = NULL;
    errors.len = 0;

    irblock_verify_ssa_based(&errors, block, path);

    return errors;
}
