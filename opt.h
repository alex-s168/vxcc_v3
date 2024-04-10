#ifndef OPT_H
#define OPT_H

#include "ssa.h"

void opt_loop_simplify(SsaView view, SsaBlock *block);

static void opt(SsaBlock *block) {
    opt_loop_simplify(ssaview_of_all(block), block);
}

#endif //OPT_H
