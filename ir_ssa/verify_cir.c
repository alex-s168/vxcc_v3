#include "cir.h"

VerifyErrors cirblock_verify(const SsaBlock *block, OpPath path) {
    VerifyErrors errors;
    errors.items = NULL;
    errors.len = 0;

    (void) block;
    (void) path;

    return errors;
}