#include "../../ir/ir.h"
#include "../../ir/llir.h"

typedef struct {
    bool use_red_zone;
} vx_cg_x86stupid;
extern vx_cg_x86stupid vx_cg_x86stupid_options;

void vx_cg_x86stupid_gen(vx_IrBlock* block, FILE* out);
