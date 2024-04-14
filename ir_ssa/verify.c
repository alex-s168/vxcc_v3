#include <stdlib.h>
#include <string.h>

#include "ssa.h"

void verifyerrors_free(const VerifyErrors errors) {
    for (size_t i = 0; i < errors.len; i ++) {
        free(errors.items[i].path.ids);
    }
    free(errors.items);
}

static OpPath oppath_copy_add(const OpPath path, const size_t id) {
    OpPath new_path = path;
    new_path.ids = realloc(new_path.ids, sizeof(size_t) * (new_path.len + 1));
    new_path.ids[new_path.len++] = id;
    return new_path;
}

static void verifyerrors_add(VerifyErrors *errors, const VerifyError *error) {
    errors->items = realloc(errors->items, sizeof(VerifyError) * (errors->len + 1));
    memcpy(&errors->items[errors->len ++], error, sizeof(VerifyError));
}

static void verifyerrors_add_all_and_free(VerifyErrors *dest, VerifyErrors *src) {
    for (size_t i = 0; i < src->len; i++) {
        verifyerrors_add(dest, &src->items[i]);
    }
    free(src->items);
    // we don't free path on purpose!
}

VerifyErrors ssablock_verify(const SsaBlock *block, const OpPath path) {
    VerifyErrors errors;
    errors.len = 0;
    errors.items = NULL;

    for (size_t i = 0; i < block->ops_len; i++) {
        const SsaOp *op = &block->ops[i];

        // TODO: implement for more types

        // verify loop state
        if (op->id == SSA_OP_FOR ||
            op->id == SSA_OP_FOREACH ||
            op->id == SSA_OP_FOREACH_UNTIL ||
            op->id == SSA_OP_REPEAT ||
            op->id == SSA_OP_INFINITE)
        {
            const size_t states_count = op->outs_len;
            for (size_t j = 0; j < states_count; j ++) {
                static char buf[256];
                sprintf(buf, "state%zu", j);
                const SsaValue *state_init = ssaop_param(op, buf);
                if (state_init == NULL) {
                    const OpPath newpath = oppath_copy_add(path, i);
                    sprintf(buf, "Loop state %zu needs to be initialized! Example: `state0=1234`", j);
                    VerifyError error = {
                        .path = newpath,
                        .error = "Loop state not initialized",
                        .additional = buf
                    };
                    verifyerrors_add(&errors, &error);
                }
            }

            const SsaValue *doblock_v = ssaop_param(op, "do");
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

        for (size_t j = 0; j < op->params_len; j ++) {
            const SsaValue val = op->params[j].val;
            if (val.type == SSA_VAL_BLOCK) {
                const OpPath newpath = oppath_copy_add(path, i);
                VerifyErrors errs = ssablock_verify(val.block, newpath);
                free(newpath.ids);
                verifyerrors_add_all_and_free(&errors, &errs);
            }
        }
    }

    return errors;
}

void verifyerrors_print(const VerifyErrors errors, FILE *dest) {
    for (size_t i = 0; i < errors.len; i ++) {
        const VerifyError err = errors.items[i];

        fputs("In operation ", dest);
        for (size_t j = 0; j < err.path.len; j ++) {
            if (j > 0)
                fputc(':', dest);
            fprintf(dest, "%zu", err.path.ids[i]);
        }

        fprintf(dest, "\n%s\n  %s\n", err.error, err.additional);
    }
}