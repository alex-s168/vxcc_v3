#include "ir.h"

void irblock_verify_ssa_based(VerifyErrors *dest, const SsaBlock *block, const OpPath path);

void error_param_type(VerifyErrors *errors, const OpPath path, const char *expected);
void error_param_missing(VerifyErrors *errors, const OpPath path, const char *param);
