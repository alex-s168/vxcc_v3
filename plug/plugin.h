#ifndef PLUGIN_H
#define PLUGIN_H

#include <stddef.h>

typedef enum {
    VX_PHASE_DESCRIPTION, // () -> char* (malloced)
    VX_PHASE_IR_PASS_BUILD, // (vx_IrPassList *) -> void
    VX_PHASE_IR_VERIFY, // (const vx_IrBlock *, const vx_OpPath *) -> Rc(vx_Errors) *
} vx_PluginPhase;

typedef void *(*vx_PluginPhaseListener)(vx_PluginPhase phase, void *a0, void *a1);

typedef struct {
    size_t id;
    const char *name;
    vx_PluginPhaseListener listener;
} vx_Plugin;

// static
vx_Plugin *vx_Plugin_register(const char *name, vx_PluginPhaseListener);
vx_Plugin *vx_Plugin_get(const char *name);

typedef struct {
    vx_Plugin *provider;
    const char *name;
} vx_IrTypeKind;

vx_IrTypeKind *vx_Plugin_registerType(vx_Plugin *plug, const char *name);

#endif //PLUGIN_H
