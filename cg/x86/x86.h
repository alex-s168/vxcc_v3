#include "../cg.h"
#include "../../ir/ir.h"

struct vx_OpInfo_X86CG_s {
    /** only for when the op is a conditional jump / move */
    vx_IrOp * cond_compare_op; // points to some optional compare instr
};

// TO BE CALLED ON FLATTENED **SSA** (with decl info)
void vx_x86cg_prepare(vx_IrBlock * block);
// TO BE CALLED ON OPTIMIZED LLIR
void vx_x86cg_regconstraints(vx_IrBlock * block);

static struct vx_OpInfo_X86CG_s* vx_x86cg_make_opinfo(void) {
    return (struct vx_OpInfo_X86CG_s*) fastalloc(sizeof(struct vx_OpInfo_X86CG_s));
}
