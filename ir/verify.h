#ifndef VERIFY_H
#define VERIFY_H

#include "ir.h"

// TODO: rename this to verify_int.h

void vx_IrBlock_verify_ssa_based(vx_CU* cu, vx_Errors *dest, vx_IrBlock *block);

void vx_error_param_type(vx_CU* cu, vx_Errors *errors, const char *expected);
void vx_error_param_missing(vx_CU* cu, vx_Errors *errors, const char *param);

#endif

