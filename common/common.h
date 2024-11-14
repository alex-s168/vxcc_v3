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

#include "../build/common/targets.cdef.h"
#include "../build/common/target_etca.cdef.h"
#include "../build/common/target_x86.cdef.h"
// add target (and create a cdef file in this dir)

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
TARGET_FLAGS_GEN(X86)
// add target

#undef TARGET_FLAGS_GEN

typedef enum {
    VX_BIN_ELF,
    VX_BIN_COFF,
    VX_BIN_MACHO,
} vx_BinFormat;

typedef struct {
    vx_TargetArch arch;
    union {
        vx_Target_ETCA__flags etca;
        vx_Target_X86__flags x86;
        // add target
    } flags;
} vx_Target;

/** format: "arch" or "arch:ext"; 0 is ok */
int vx_Target_parse(vx_Target* dest, const char * str);

#endif //COMMON_H
