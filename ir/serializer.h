#include <stdint.h>

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


