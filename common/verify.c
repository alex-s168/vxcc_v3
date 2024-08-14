#include <stdlib.h>
#include <string.h>

#include "../common/common.h"

void vx_Errors_free(const vx_Errors errors) {
    free(errors.items);
}

void vx_Errors_add(vx_Errors *errors, const vx_Error *error) {
    errors->items = realloc(errors->items, sizeof(vx_Error) * (errors->len + 1));
    memcpy(&errors->items[errors->len ++], error, sizeof(vx_Error));
}

void vx_Errors_add_all_and_free(vx_Errors *dest, vx_Errors *src) {
    for (size_t i = 0; i < src->len; i++) {
        vx_Errors_add(dest, &src->items[i]);
    }
    free(src->items);
    // we don't free path on purpose!
}

void vx_Errors_print(const vx_Errors errors, FILE *dest) {
    for (size_t i = 0; i < errors.len; i ++) {
        const vx_Error err = errors.items[i];

        fputs("In operation ", dest);
        fprintf(dest, "\n%s\n  %s\n", err.error, err.additional);
    }
}
