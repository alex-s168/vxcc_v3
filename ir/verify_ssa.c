#include <assert.h>
#include <stdlib.h>

#include "ir.h"

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

static void error_param_type(VerifyErrors *errors, const OpPath path, const char *expected) {
    static char buf[256];
    sprintf(buf, "Expected parameter type %s", expected);
    const VerifyError error = {
        .path = path,
        .error = "Incorrect type",
        .additional = buf
    };
    verifyerrors_add(errors, &error);
}

static void error_param_missing(VerifyErrors *errors, const OpPath path, const char *param) {
    static char buf[256];
    sprintf(buf, "Missing required parameter %s", param);
    const VerifyError error = {
        .path = path,
        .error = "Missing parameter",
        .additional = buf
    };
    verifyerrors_add(errors, &error);
}

VerifyErrors irblock_verify(const SsaBlock *block, const OpPath path) {
    VerifyErrors errors;
    errors.len = 0;
    errors.items = NULL;

    const SsaBlock *root = irblock_root(block);

    for (size_t i = 0; i < block->ops_len; i++) {
        const SsaOp *op = &block->ops[i];

        // TODO: implement for more types

        // verify states in general
        for (size_t j = 0; j < op->states_len; j ++) {
            if (op->states[j].type == SSA_VAL_BLOCK) {
                const OpPath newpath = oppath_copy_add(path, i);
                const VerifyError error = {
                    .path = newpath,
                    .error = "Invalid state",
                    .additional = "States cannot be blocks!"
                };
                verifyerrors_add(&errors, &error);
            }
        }

        // verify loop state
        if (op->id == SSA_OP_FOR ||
            op->id == SSA_OP_FOREACH ||
            op->id == SSA_OP_FOREACH_UNTIL ||
            op->id == SSA_OP_REPEAT ||
            op->id == SSA_OP_INFINITE ||
            op->id == SSA_OP_WHILE)
        {
            const size_t states_count = op->outs_len;
            if (states_count != op->states_len) {
                const OpPath newpath = oppath_copy_add(path, i);
                VerifyError error = {
                    .path = newpath,
                    .error = "Loop state not initialized",
                    .additional = "Loop states do not match! Example: `state0=1234`"
                };
                verifyerrors_add(&errors, &error);
            }

            const SsaValue *doblock_v = irop_param(op, SSA_NAME_LOOP_DO);
            if (doblock_v == NULL) {
                const OpPath newpath = oppath_copy_add(path, i);
                VerifyError error = {
                    .path = newpath,
                    .error = "Loop is missing a do block",
                    .additional = "Loop is missing a `do` block. Example: `do=(%0){}`"
                };
                verifyerrors_add(&errors, &error);
            }
            else {
                if (doblock_v->type != SSA_VAL_BLOCK) {
                    error_param_type(&errors, oppath_copy_add(path, i), "block");
                } else {
                    const SsaBlock *doblock = doblock_v->block;

                    if (doblock->outs_len != states_count) {
                        const OpPath newpath = oppath_copy_add(path, i);
                        VerifyError error = {
                            .path = newpath,
                            .error = "Do block is missing state changes",
                            .additional = "Loop do-block is missing state-updates for all states!"
                        };
                        verifyerrors_add(&errors, &error);
                    }
                }
            }
        }

        if (op->id == SSA_OP_IF) {
            bool err = false;

            const SsaValue *vcond = irop_param(op, SSA_NAME_COND);
            if (vcond == NULL) {
                error_param_missing(&errors, oppath_copy_add(path, i), "cond");
                err = true;
            } else if (vcond->type != SSA_VAL_BLOCK) {
                error_param_type(&errors, oppath_copy_add(path, i), "block");
                err = true;
            }

            const SsaValue *vthen = irop_param(op, SSA_NAME_COND_THEN);
            if (vthen == NULL) {
                error_param_missing(&errors, oppath_copy_add(path, i), "then");
                err = true;
            } else if (vthen->type != SSA_VAL_BLOCK) {
                error_param_type(&errors, oppath_copy_add(path, i), "block");
                err = true;
            }

            const SsaValue *velse = irop_param(op, SSA_NAME_COND_ELSE);
            if (velse == NULL) {
                error_param_missing(&errors, oppath_copy_add(path, i), "else");
                err = true;
            } else if (velse->type != SSA_VAL_BLOCK) {
                error_param_type(&errors, oppath_copy_add(path, i), "block");
                err = true;
            }

            if (!err) {
                SsaBlock *bcond = vcond->block;
                SsaBlock *bthen = vthen->block;
                SsaBlock *belse = velse->block;

                if (bcond->outs_len != 1) {
                    const OpPath newpath = oppath_copy_add(path, i);
                    VerifyError error = {
                        .path = newpath,
                        .error = "If block is missing a condition",
                        .additional = "If block is missing a condition! Example: `cond=(){ ^ 1 }`"
                    };
                    verifyerrors_add(&errors, &error);
                }

                if (!(bthen->outs_len == belse->outs_len && bthen->outs_len == op->outs_len)) {
                    const OpPath newpath = oppath_copy_add(path, i);
                    VerifyError error = {
                        .path = newpath,
                        .error = "If block invalid outputs",
                        .additional = "Expected every branch in the if block to have the same amount of outputs!"
                    };
                    verifyerrors_add(&errors, &error);
                }
            }
        }

        for (size_t j = 0; j < op->params_len; j ++) {
            const SsaValue val = op->params[j].val;

            if (val.type == SSA_VAL_BLOCK) {
                const OpPath newpath = oppath_copy_add(path, i);
                VerifyErrors errs = irblock_verify(val.block, newpath);
                free(newpath.ids);
                verifyerrors_add_all_and_free(&errors, &errs);
            }
            else if (val.type == SSA_VAL_VAR) {
                const SsaVar var = val.var;
                if (var >= root->as_root.vars_len) {
                    static char buf[256];
                    sprintf(buf, "Variable %%%zu id is bigger than root block variable count - 1!", var);
                    const OpPath newpath = oppath_copy_add(path, i);
                    VerifyError error = {
                        .path = newpath,
                        .error = "Variable out of bounds",
                        .additional = buf
                    };
                    verifyerrors_add(&errors, &error);
                } else if (root->as_root.vars[var].decl == NULL) {
                    static char buf[256];
                    sprintf(buf, "Variable %%%zu is never declared!", var);
                    const OpPath newpath = oppath_copy_add(path, i);
                    VerifyError error = {
                        .path = newpath,
                        .error = "Undeclared variable",
                        .additional = buf
                    };
                    verifyerrors_add(&errors, &error);
                }
            }
        }
    }

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