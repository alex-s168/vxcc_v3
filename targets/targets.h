#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#include "../build/targets/targets.cdef.h"
#include "../build/targets/etca/etca.cdef.h"
#include "../build/targets/x86/x86.cdef.h"
// add target (and create a cdef file)

#define TARGET_FLAGS_GEN(tg) \
typedef bool vx_Target_##tg##__flags [vx_Target_##tg##__len]; \
void vx_Target_##tg##__parseAdditionalFlags(vx_Target_##tg##__flags * dest, const char * c); \
void vx_Target_##tg##__parseAdditionalFlag(vx_Target_##tg##__flags * dest, const char * flag); \

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
