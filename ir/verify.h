#ifndef VERIFY_H
#define VERIFY_H

#include "ir.h"

void vx_IrBlock_verify_ssa_based(vx_Errors *dest, const vx_IrBlock *block, vx_OpPath path);

void vx_error_param_type(vx_Errors *errors, vx_OpPath path, const char *expected);
void vx_error_param_missing(vx_Errors *errors, vx_OpPath path, const char *param);

#endif

