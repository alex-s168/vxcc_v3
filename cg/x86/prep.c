#include "x86.h"
#include <assert.h>

static bool is_compare(vx_IrOp *op) {
    switch (op->id) {
    case VX_IR_OP_EQ:
    case VX_IR_OP_NEQ:
    case VX_IR_OP_GT:
    case VX_IR_OP_LT:
    case VX_IR_OP_GTE:
    case VX_IR_OP_LTE:
        return true;

    default:
        return false;
    }
}

static void add_cond_comp_info(vx_IrBlock * block) {
    for (size_t i = 0; i < block->ops_len; i ++) {
        vx_IrOp *op = &block->ops[i];

        if (!(op->id == VX_IR_OP_CMOV || op->id == VX_LIR_GOTO || op->id == VX_LIR_COND))
            continue;

        vx_IrVar cond = vx_IrOp_param(op, VX_IR_NAME_COND)->var;

        vx_IrOp *compare = vx_IrBlock_root_get_var_decl(block, cond);
        if (!compare)
            continue;

        if (!is_compare(compare))
            continue;

        struct vx_OpInfo_X86CG_s* infop = vx_x86cg_make_opinfo();

        infop->cond_compare_op = compare;
        
        vx_OpInfo info;
        info.kind = VX_OPINFO_X86CG;
        info.value.x86 = infop;
        vx_OpInfoList_add(&op->info, info);
    }
}

// called on flattened **ssa**
void vx_x86cg_prepare(vx_IrBlock * block) {
    assert(block->is_root);

    add_cond_comp_info(block);
}
