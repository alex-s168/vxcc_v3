#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "verify.h"

void vx_error_param_type(vx_Errors *errors, const char *expected) {
    static char buf[256];
    sprintf(buf, "Expected parameter type %s", expected);
    const vx_Error error = {
        .error = "Incorrect type",
        .additional = buf
    };
    vx_Errors_add(errors, &error);
}

void vx_error_param_missing(vx_Errors *errors, const char *param) {
    static char buf[256];
    sprintf(buf, "Missing required parameter %s", param);
    const vx_Error error = {
        .error = "Missing parameter",
        .additional = buf
    };
    vx_Errors_add(errors, &error);
}

static bool analyze_if(vx_Errors *dest,
                       const vx_IrOp *op)
{
    bool err = false;

    const vx_IrValue *vcond = vx_IrOp_param(op, VX_IR_NAME_COND);
    if (vcond == NULL) {
        vx_error_param_missing(dest, "cond");
        err = true;
    }

    const vx_IrValue *vthen = vx_IrOp_param(op, VX_IR_NAME_COND_THEN);
    if (vthen == NULL) {
        vx_error_param_missing(dest, "then");
        err = true;
    } else if (vthen->type != VX_IR_VAL_BLOCK) {
        vx_error_param_type(dest, "block");
        err = true;
    }

    const vx_IrValue *velse = vx_IrOp_param(op, VX_IR_NAME_COND_ELSE);
    if (velse == NULL) {
        vx_error_param_missing(dest, "else");
        err = true;
    } else if (velse->type != VX_IR_VAL_BLOCK) {
        vx_error_param_type(dest, "block");
        err = true;
    }

    if (!err) {
        vx_IrBlock *bthen = vthen->block;
        vx_IrBlock *belse = velse->block;

        if (!(bthen->outs_len == belse->outs_len && bthen->outs_len == op->outs_len)) {
            vx_Error error = {
                .error = "If block invalid outputs",
                .additional = "Expected every branch in the if block to have the same amount of outputs!"
            };
            vx_Errors_add(dest, &error);
        }
    }

    return err;
}

static void analyze_loops(vx_Errors *dest,
                          const vx_IrOp *op)
{
    const size_t states_count = op->outs_len;
    if (states_count != op->args_len) {
        vx_Error error = {
            .error = "Loop state not initialized",
            .additional = "Loop states do not match! Example: `state0=1234`"
        };
        vx_Errors_add(dest, &error);
    }

    const vx_IrValue *doblock_v = vx_IrOp_param(op, VX_IR_NAME_LOOP_DO);
    if (doblock_v == NULL) {
        vx_Error error = {
            .error = "Loop is missing a do block",
            .additional = "Loop is missing a `do` block. Example: `do=(%0){}`"
        };
        vx_Errors_add(dest, &error);
    }
    else {
        if (doblock_v->type != VX_IR_VAL_BLOCK) {
            vx_error_param_type(dest, "block");
        } else {
            const vx_IrBlock *doblock = doblock_v->block;

            if (doblock->outs_len != states_count) {
                vx_Error error = {
                    .error = "Do block is missing state changes",
                    .additional = "Loop do-block is missing state-updates for all states!"
                };
                vx_Errors_add(dest, &error);
            }
        }
    }
}

void vx_IrBlock_verify_ssa_based(vx_Errors *dest, vx_IrBlock *block) {
    vx_IrBlock *root = vx_IrBlock_root(block);

    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (op->parent != block) {
            char buf[256];
            sprintf(buf, "OP (%s) has different parent", vx_IrOpType__entries[op->id].debug.a);
            const vx_Error error = {
                .error = "Invalid parent",
                .additional = strdup(buf)
            };
            vx_Errors_add(dest, &error);
        }

        // verify states in general
        for (size_t j = 0; j < op->args_len; j ++) {
            if (op->args[j].type == VX_IR_VAL_BLOCK ||
                op->args[j].type == VX_IR_VAL_TYPE)
            {
                const vx_Error error = {
                    .error = "Invalid state",
                    .additional = "States can only be values that are present at runtime!"
                };
                vx_Errors_add(dest, &error);
            }
        }

        // TODO: implement for more ops
        switch (op->id) {
            case VX_IR_OP_FOR:
            case VX_IR_OP_REPEAT:
            case VX_IR_OP_INFINITE:
            case VX_IR_OP_WHILE:
                analyze_loops(dest, op);
                break;

            case VX_IR_OP_IF:
                (void) analyze_if(dest, op);
                break;

            case VX_IR_OP_GOTO:
            case VX_IR_OP_COND: {
                size_t label = vx_IrOp_param(op, VX_IR_NAME_ID)->id;
                if (label >= root->as_root.labels_len || root->as_root.labels[label].decl == NULL) {
                    char buf[256];
                    sprintf(buf, "Label %%%zu is never declared!", label);
                    vx_Error error = {
                        .error = "Undeclared label",
                        .additional = strdup(buf)
                    };
                    vx_Errors_add(dest, &error);
                }
            } break;

            case VX_IR_OP_CALL:
            case VX_IR_OP_CONDTAILCALL: {
                vx_IrValue addr = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
                vx_IrTypeRef ty = vx_IrValue_type((vx_IrBlock*) root, addr);
                assert(ty.ptr);
                assert(ty.ptr->kind == VX_IR_TYPE_FUNC);
                vx_IrTypeRef_drop(ty);
            } break;

            default:
                break;
        }

        FOR_INPUTS(op, val, ({
            if (val.type == VX_IR_VAL_BLOCK) {
                if (val.block->parent != block) {
                    const vx_Error error = {
                        .error = "Invalid parent",
                        .additional = "Block has different parent"
                    };
                    vx_Errors_add(dest, &error);
                }
                vx_Errors errs = vx_IrBlock_verify(val.block);
                vx_Errors_add_all_and_free(dest, &errs);
            }
            else if (val.type == VX_IR_VAL_VAR) {
                const vx_IrVar var = val.var;
                bool found = vx_IrBlock_varDeclRec(root, var) || vx_IrBlock_vardeclIsInIns(block, var);
                if (!found) {
                    static char buf[256];
                    sprintf(buf, "Variable %%%zu is never declared!", var);
                    vx_Error error = {
                        .error = "Undeclared variable",
                        .additional = strdup(buf)
                    };
                    vx_Errors_add(dest, &error);
                }
            }
        }))
    }
}
