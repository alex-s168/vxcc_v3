#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
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

#define OPT_PASS        /* opt: */
#define TRANSFORM_PASS  /* transform: */
#define ANALYSIS_PASS   /* analysis: */

#include "../build/common/targets.cdef.h"

#define TARGET_FLAGS_GEN(tg) \
typedef bool vx_Target_##tg##__flags [vx_Target_##tg##__len]; \
static void vx_Target_##tg##__parseAdditionalFlags(vx_Target_##tg##__flags * dest, const char * c); \
static void vx_Target_##tg##__parseAdditionalFlag(vx_Target_##tg##__flags * dest, const char * flag) { \
    for (size_t i = 0; i < vx_Target_##tg##__len; i ++) { \
        if (*dest[i]) continue; \
        vx_Target_##tg##__entry* entry = &vx_Target_##tg##__entries[i]; \
        if (strcmp(entry->name.a, flag) == 0) { \
            (*dest)[i] = true; \
            if (entry->infer.set) \
                vx_Target_##tg##__parseAdditionalFlags(dest, entry->infer.a); \
            return; \
        } \
    } \
} \
static void vx_Target_##tg##__parseAdditionalFlags(vx_Target_##tg##__flags * dest, const char * c) { \
    while (c) { \
        size_t len = strchr(c, ',') ? (strchr(c, ',') - c) : strlen(c); \
        char copy[32]; \
        if (len > 31) \
            len = 31; \
        memcpy(copy, c, len + 1); \
        copy[31] = '\0'; \
        vx_Target_##tg##__parseAdditionalFlag(dest, copy); \
        c = strchr(c, ','); \
    } \
}

TARGET_FLAGS_GEN(ETCA)

typedef enum {
    // only compile to llir and then output 
    VX_TARGET_VXLLIR,

    // only compile to ssair and then output 
    VX_TARGET_SSAIR,

    VX_TARGET_X86_64,
    VX_TARGET_ETCA,
} vx_TargetArch;

typedef struct {
    vx_TargetArch arch;
    union {
        vx_Target_ETCA__flags etca;
    } flags;
} vx_Target;

#endif //COMMON_H
