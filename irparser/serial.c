#include "../common/serializer.h"
#include "parser.h"

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
