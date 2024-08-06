#ifndef VX_CG_H
#define VX_CG_H

#include <stddef.h>
#include <stdlib.h>

#ifndef VX_BASE_TYPES
#define VX_BASE_TYPES
typedef size_t vx_IrVar;
#endif

typedef struct vx_OpInfo_X86CG_s * vx_OpInfo_X86CG;

typedef enum {
    VXBOPINFO_NONE,
    VX_OPINFO_X86CG,
} vx_OpInfoKind;

typedef struct {
    vx_OpInfoKind kind;
    union {
        vx_OpInfo_X86CG x86;
} value;
} vx_OpInfo;

typedef struct {
    vx_OpInfo * items;
    size_t      count;
} vx_OpInfoList;

static vx_OpInfoList vx_OpInfoList_create(void) {
    vx_OpInfoList list;
    list.items = NULL;
    list.count = 0;
    return list;
}

static void vx_OpInfoList_destroy(vx_OpInfoList * list) {
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

static void vx_OpInfoList_add(vx_OpInfoList * list, vx_OpInfo info) {
    list->items = (vx_OpInfo *)
        realloc(list->items, sizeof(vx_OpInfo) * (list->count + 1));
    list->items[list->count ++] = info;
}

static void * vx_OpInfo_lookup__impl(vx_OpInfoList list, vx_OpInfoKind kind) {
    for (size_t i = 0; i < list.count; i ++) {
        vx_OpInfo *item = &list.items[i];
        if (item->kind == kind) {
            return &item->value;
        }
    }
    return NULL;
}

// example usage:  vx_OpInfo_lookup(list, X86CG)
#define vx_OpInfo_lookup(list, kind) ((vx_OpInfo_##kind *) vx_OpInfo_lookup__impl(list, VX_OPINFO_##kind))

typedef size_t vx_CgReg;
#define VX_CGREG_NONE ((vx_CgReg) 0)

typedef struct {
    vx_CgReg (*allocRegFor)(struct vx_IrBlock_s *root, vx_IrVar var);
    vx_CgReg (*allocArgReg)(struct vx_IrBlock_s *root, size_t id);
    vx_CgReg (*allocRetReg)(struct vx_IrBlock_s *root, size_t id);
    void     (*freeRe)(vx_CgReg);

    union {
        struct vx_X86CgState * x86;
    };
} vx_CgState;

#endif
