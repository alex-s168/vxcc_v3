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

#endif //COMMON_H
