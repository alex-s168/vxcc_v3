#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    size_t *ids;
    size_t  len;
} OpPath;

typedef struct {
    OpPath path;

    const char *error;
    const char *additional;
} VerifyError;

typedef struct {
    VerifyError *items;
    size_t       len;
} VerifyErrors;

void verifyerrors_free(VerifyErrors errors);
void verifyerrors_print(VerifyErrors errors, FILE *dest);
OpPath oppath_copy_add(OpPath path, size_t id);
void verifyerrors_add(VerifyErrors *errors, const VerifyError *error);
void verifyerrors_add_all_and_free(VerifyErrors *dest, VerifyErrors *src);

#endif //COMMON_H
