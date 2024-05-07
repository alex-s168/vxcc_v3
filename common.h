#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    size_t *ids;
    size_t  len;
} vx_OpPath;

vx_OpPath vx_OpPath_copy_add(vx_OpPath path, size_t id);

typedef struct {
    vx_OpPath path;

    const char *error;
    const char *additional;
} vx_Error;

typedef struct {
    vx_Error *items;
    size_t       len;
} vx_Errors;

void vx_Errors_add(vx_Errors *errors, const vx_Error *error);
void vx_Errors_add_all_and_free(vx_Errors *dest, vx_Errors *src);
void vx_Errors_free(vx_Errors errors);
void vx_Errors_print(vx_Errors errors, FILE *dest);

#endif //COMMON_H
