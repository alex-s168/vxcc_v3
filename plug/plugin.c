#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "plugin.h"

static vx_Plugin *plugins = NULL;
static size_t plugins_len = 0;

vx_Plugin *vx_Plugin_register(const char *name, vx_PluginPhaseListener listener) {
    assert(listener != NULL);
    
    plugins = realloc(plugins, sizeof(vx_Plugin) * (plugins_len + 1));
    vx_Plugin *plug = &plugins[plugins_len];
    
    plug->id = plugins_len ++;
    plug->name = name;
    plug->listener = listener;

    return plug;
}

vx_Plugin *vx_Plugin_get(const char *name) {
    for (size_t i = 0; i < plugins_len; i ++)
        if (strcmp(plugins[i].name, name) == 0)
            return &plugins[i];
    return NULL;
}

vx_IrTypeKind *vx_Plugin_registerType(vx_Plugin *plug, const char *name);
