#include <assert.h>

#include "../opt.h"

// we go trough all ops between the jump and the label
// if all of them have no effect (labels, nops), we can remove the jump instruction
static void part1(vx_IrBlock *block) {
    vx_IrBlock *root = (vx_IrBlock*)vx_IrBlock_root(block);
    for (size_t opid = 0; opid < block->ops_len; opid ++) {
        vx_IrOp *op = &block->ops[opid];

        if (!(op->id == VX_LIR_GOTO || op->id == VX_LIR_COND))
            continue;

        size_t label_id = vx_IrOp_param(op, VX_IR_NAME_ID)->id;

        vx_IrOp *decl = vx_IrBlock_root_get_label_decl(root, label_id);

        if (decl->parent != op->parent)
            continue;

        if (decl < op) // can't optimize if label decl before this decl
            continue;
        
        bool can_opt = true;
        for (size_t i = (op - block->ops) + 1; i < (size_t) (decl - block->ops); i ++) {
            if (vx_IrOpType_has_effect(block->ops[i].id)) {
                can_opt = false;
                break;
            }
        }

        if (!can_opt)
            continue;

        vx_IrOp_destroy(op);

        vx_IrOp_init(op, VX_IR_OP_NOP, block);
    }
}

// is the jump dest just another jump? optimize that
static void part2(vx_IrBlock *block) {
    vx_IrBlock *root = (vx_IrBlock*)vx_IrBlock_root(block);
    for (size_t opid = 0; opid < block->ops_len; opid ++) {
        vx_IrOp *op = &block->ops[opid];

        if (!(op->id == VX_LIR_GOTO || op->id == VX_LIR_COND))
            continue;

        size_t label_id = vx_IrOp_param(op, VX_IR_NAME_ID)->id;

        vx_IrOp *decl = vx_IrBlock_root_get_label_decl(root, label_id);
        if (decl->parent != block)
            continue;
        size_t decl_id = decl - block->ops;

        if (decl_id + 1 < block->ops_len) {
            vx_IrOp *following = &block->ops[decl_id + 1];

            if (following->id == VX_LIR_GOTO) {
                label_id = vx_IrOp_param(following, VX_IR_NAME_ID)->id;
                vx_IrOp_param(op, VX_IR_NAME_ID)->id = label_id;
            }
        }
    }
}

void vx_opt_ll_jumps(vx_IrView view, vx_IrBlock *block) {
    assert(view.block == block);
    part1(block);
    part2(block);
}

