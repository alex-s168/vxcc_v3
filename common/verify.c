#include <stdlib.h>
#include <string.h>

#include "../common.h"

void verifyerrors_free(const VerifyErrors errors) {
    for (size_t i = 0; i < errors.len; i ++) {
        free(errors.items[i].path.ids);
    }
    free(errors.items);
}

OpPath oppath_copy_add(const OpPath path, const size_t id) {
    OpPath new_path = path;
    new_path.ids = malloc(sizeof(size_t) * (new_path.len + 1));
    memcpy(new_path.ids, path.ids, sizeof(size_t) * new_path.len);
    new_path.ids[new_path.len++] = id;
    return new_path;
}

void verifyerrors_add(VerifyErrors *errors, const VerifyError *error) {
    errors->items = realloc(errors->items, sizeof(VerifyError) * (errors->len + 1));
    memcpy(&errors->items[errors->len ++], error, sizeof(VerifyError));
}

void verifyerrors_add_all_and_free(VerifyErrors *dest, VerifyErrors *src) {
    for (size_t i = 0; i < src->len; i++) {
        verifyerrors_add(dest, &src->items[i]);
    }
    free(src->items);
    // we don't free path on purpose!
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
