#include <stdlib.h>

#include "verify.h"

void vx_error_param_type(vx_Errors *errors, const vx_OpPath path, const char *expected) {
    static char buf[256];
    sprintf(buf, "Expected parameter type %s", expected);
    const vx_Error error = {
        .path = path,
        .error = "Incorrect type",
        .additional = buf
    };
    vx_Errors_add(errors, &error);
}

void vx_error_param_missing(vx_Errors *errors, const vx_OpPath path, const char *param) {
    static char buf[256];
    sprintf(buf, "Missing required parameter %s", param);
    const vx_Error error = {
        .path = path,
        .error = "Missing parameter",
        .additional = buf
    };
    vx_Errors_add(errors, &error);
}

static bool analyze_if(vx_Errors *dest,
                       const vx_IrOp *op,
                       size_t i,
                       vx_OpPath path)
{
    bool err = false;

    const vx_IrValue *vcond = vx_IrOp_param(op, VX_IR_NAME_COND);
    if (vcond == NULL) {
        vx_error_param_missing(dest, vx_OpPath_copy_add(path, i), "cond");
        err = true;
    } else if (vcond->type != VX_IR_VAL_BLOCK) {
        vx_error_param_type(dest, vx_OpPath_copy_add(path, i), "block");
        err = true;
    }

    const vx_IrValue *vthen = vx_IrOp_param(op, VX_IR_NAME_COND_THEN);
    if (vthen == NULL) {
        vx_error_param_missing(dest, vx_OpPath_copy_add(path, i), "then");
        err = true;
    } else if (vthen->type != VX_IR_VAL_BLOCK) {
        vx_error_param_type(dest, vx_OpPath_copy_add(path, i), "block");
        err = true;
    }

    const vx_IrValue *velse = vx_IrOp_param(op, VX_IR_NAME_COND_ELSE);
    if (velse == NULL) {
        vx_error_param_missing(dest, vx_OpPath_copy_add(path, i), "else");
        err = true;
    } else if (velse->type != VX_IR_VAL_BLOCK) {
        vx_error_param_type(dest, vx_OpPath_copy_add(path, i), "block");
        err = true;
    }

    if (!err) {
        vx_IrBlock *bcond = vcond->block;
        vx_IrBlock *bthen = vthen->block;
        vx_IrBlock *belse = velse->block;

        if (bcond->outs_len != 1) {
            const vx_OpPath newpath = vx_OpPath_copy_add(path, i);
            vx_Error error = {
                .path = newpath,
                .error = "If block is missing a condition",
                .additional = "If block is missing a condition! Example: `cond=(){ ^ 1 }`"
            };
            vx_Errors_add(dest, &error);
        }

        if (!(bthen->outs_len == belse->outs_len && bthen->outs_len == op->outs_len)) {
            const vx_OpPath newpath = vx_OpPath_copy_add(path, i);
            vx_Error error = {
                .path = newpath,
                .error = "If block invalid outputs",
                .additional = "Expected every branch in the if block to have the same amount of outputs!"
            };
            vx_Errors_add(dest, &error);
        }
    }

    return err;
}

static void analyze_loops(vx_Errors *dest,
                          const vx_IrOp *op,
                          size_t i,
                          vx_OpPath path)
{
    const size_t states_count = op->outs_len;
    if (states_count != op->states_len) {
        const vx_OpPath newpath = vx_OpPath_copy_add(path, i);
        vx_Error error = {
            .path = newpath,
            .error = "Loop state not initialized",
            .additional = "Loop states do not match! Example: `state0=1234`"
        };
        vx_Errors_add(dest, &error);
    }

    const vx_IrValue *doblock_v = vx_IrOp_param(op, VX_IR_NAME_LOOP_DO);
    if (doblock_v == NULL) {
        const vx_OpPath newpath = vx_OpPath_copy_add(path, i);
        vx_Error error = {
            .path = newpath,
            .error = "Loop is missing a do block",
            .additional = "Loop is missing a `do` block. Example: `do=(%0){}`"
        };
        vx_Errors_add(dest, &error);
    }
    else {
        if (doblock_v->type != VX_IR_VAL_BLOCK) {
            vx_error_param_type(dest, vx_OpPath_copy_add(path, i), "block");
        } else {
            const vx_IrBlock *doblock = doblock_v->block;

            if (doblock->outs_len != states_count) {
                const vx_OpPath newpath = vx_OpPath_copy_add(path, i);
                vx_Error error = {
                    .path = newpath,
                    .error = "Do block is missing state changes",
                    .additional = "Loop do-block is missing state-updates for all states!"
                };
                vx_Errors_add(dest, &error);
            }
        }
    }
}

void vx_IrBlock_verify_ssa_based(vx_Errors *dest, const vx_IrBlock *block, const vx_OpPath path) {
    const vx_IrBlock *root = vx_IrBlock_root(block);

    for (size_t i = 0; i < block->ops_len; i++) {
        const vx_IrOp *op = &block->ops[i];

        if (op->parent != block) {
            const vx_OpPath newpath = vx_OpPath_copy_add(path, i);
            const vx_Error error = {
                .path = newpath,
                .error = "Invalid parent",
                .additional = "OP has different parent"
            };
            vx_Errors_add(dest, &error);
        }

        // verify states in general
        for (size_t j = 0; j < op->states_len; j ++) {
            if (op->states[j].type == VX_IR_VAL_BLOCK ||
                op->states[j].type == VX_IR_VAL_TYPE)
            {
                const vx_OpPath newpath = vx_OpPath_copy_add(path, i);
                const vx_Error error = {
                    .path = newpath,
                    .error = "Invalid state",
                    .additional = "States can only be values that are present at runtime!"
                };
                vx_Errors_add(dest, &error);
            }
        }

        // TODO: implement for more ops
        switch (op->id) {
        case VX_IR_OP_FOR:
        case VX_IR_OP_FOREACH:
        case VX_IR_OP_FOREACH_UNTIL:
        case VX_IR_OP_REPEAT:
        case VX_IR_OP_INFINITE:
        case VX_IR_OP_WHILE:
            analyze_loops(dest, op, i, path);
            break;

        case VX_IR_OP_IF:
            (void) analyze_if(dest, op, i, path);
            break;

        default:
            break;
        }

        for (size_t j = 0; j < op->params_len; j ++) {
            const vx_IrValue val = op->params[j].val;

            if (val.type == VX_IR_VAL_BLOCK) {
                const vx_OpPath newpath = vx_OpPath_copy_add(path, i);
                vx_Errors errs = vx_IrBlock_verify(val.block, newpath);
                free(newpath.ids);
                vx_Errors_add_all_and_free(dest, &errs);
            }
            else if (val.type == VX_IR_VAL_VAR) {
                const vx_IrVar var = val.var;
                if (var >= root->as_root.vars_len) {
                    static char buf[256];
                    sprintf(buf, "Variable %%%zu id is bigger than root block variable count - 1!", var);
                    const vx_OpPath newpath = vx_OpPath_copy_add(path, i);
                    vx_Error error = {
                        .path = newpath,
                        .error = "Variable out of bounds",
                        .additional = buf
                    };
                    vx_Errors_add(dest, &error);
                } else if (root->as_root.vars[var].decl_parent == NULL) {
                    bool found = false;
                    for (size_t i = 0; i < root->ins_len; i ++) {
                        if (root->ins[i] == var) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        static char buf[256];
                        sprintf(buf, "Variable %%%zu is never declared!", var);
                        const vx_OpPath newpath = vx_OpPath_copy_add(path, i);
                        vx_Error error = {
                            .path = newpath,
                            .error = "Undeclared variable",
                            .additional = buf
                        };
                        vx_Errors_add(dest, &error);
                    }
                }
            }
        }
    }
}
