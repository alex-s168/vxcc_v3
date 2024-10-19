#include <stdint.h>
#include "parser.h"

#define WRITE(sz, src) \
    { if (destzu < (sz)) return 0; \
      destzu -= (sz);              \
      memcpy(dest, src, (sz)); }

#define WRITEV(v) \
    WRITE(sizeof(v), &v)

#define WRITEI(ty, value) \
    { ty __val = (value); WRITEV(__val); }

#define WRITES(str, len) \
    { uint32_t _lc = (len); WRITEV(_lc); WRITE(_lc, str); }

#define WRITEGS(gs) \
    if (germanstr_is_long((gs))) WRITES((gs)._long.ptr, (gs).len) \
    else WRITES((gs)._short.buf, (gs).len)

#define WR(fn, arg) \
    { size_t _wr = fn(arg, dest, destzu); if (_wr == 0) return 0; destzu -= _wr; }

#define SERIALI(name, arg, body) \
size_t name(arg, void* dest, size_t destzu) { \
    size_t oldDZ = destzu; \
    body; \
    return destzu - oldDZ; \
}

SERIALI(CompOperation_serialize, CompOperation const* o,
({
    WRITEI(uint8_t, (uint8_t) o->is_any);
    if (!o->is_any)
    {
        WRITEI(uint32_t, (uint32_t) o->specific.op_type);

        WRITEI(uint32_t, o->specific.outputs.count);
        for (uint32_t i = 0; i < o->specific.outputs.count; i ++)
            WRITEI(uint32_t, o->specific.outputs.items[i]);

        WRITEI(uint32_t, o->specific.operands.count);
        for (uint32_t i = 0; i < o->specific.operands.count; i ++)
            WRITEV(o->specific.operands.items[i]);
    }
}));

/** if return 0, not enough space */
SERIALI(Pattern_serialize, CompPattern pat,
({
    WRITEI(uint32_t, pat.placeholders_count);
    for (uint32_t i = 0; i < pat.placeholders_count; i ++)
        WRITEGS(pat.placeholders[i]);

    WRITEI(uint32_t, pat.count);
    for (uint32_t i = 0; i < pat.count; i ++)
        WR(CompOperation_serialize, &pat.items[i]);
}));
