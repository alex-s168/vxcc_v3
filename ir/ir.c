#include "ir.h"

#include <assert.h>
#include <stdlib.h>

// TODO: add boolean to stop traverse
void vx_IrView_deep_traverse(vx_IrView top, void (*callback)(vx_IrOp *op, void *data), void *data) {
    for (size_t i = top.start; i < top.end; i ++) {
        vx_IrOp *op = &top.block->ops[i];

        for (size_t j = 0; j < op->params_len; j ++)
            if (op->params[j].val.type == VX_IR_VAL_BLOCK)
                vx_IrView_deep_traverse(vx_IrView_of_all(op->params[j].val.block), callback, data);

        callback(op, data);
    }
}

const vx_IrBlock *vx_IrBlock_root(const vx_IrBlock *block) {
    while (block != NULL && !block->is_root) {
        block = block->parent;
    }
    return block;
}

void vx_IrBlock_swap_in_at(vx_IrBlock *block, const size_t a, const size_t b) {
    if (a == b)
        return;

    const vx_IrVar va = block->ins[a];
    block->ins[a] = block->ins[b];
    block->ins[b] = va;
}

void vx_IrBlock_swap_out_at(vx_IrBlock *block, size_t a, size_t b) {
    if (a == b)
        return;

    const vx_IrVar va = block->outs[a];
    block->outs[a] = block->outs[b];
    block->outs[b] = va;
}

vx_IrVar vx_IrBlock_new_var(vx_IrBlock *block, vx_IrOp *decl) {
    assert(block != NULL);
    assert(decl != NULL);
    vx_IrBlock *root = (vx_IrBlock *) vx_IrBlock_root(block);
    assert(root != NULL);
    root->as_root.vars = realloc(root->as_root.vars, (root->as_root.vars_len + 1) * sizeof(*root->as_root.vars));
    vx_IrVar new = root->as_root.vars_len ++;
    root->as_root.vars[new].decl = decl;
    return new;
}

size_t vx_IrBlock_new_label(vx_IrBlock *block, vx_IrOp *decl) {
    assert(block != NULL);
    assert(decl != NULL);
    vx_IrBlock *root = (vx_IrBlock *) vx_IrBlock_root(block);
    assert(root != NULL);
    root->as_root.labels = realloc(root->as_root.labels, (root->as_root.labels_len + 1) * sizeof(*root->as_root.labels));
    size_t new = root->as_root.labels_len ++;
    root->as_root.labels[new].decl = decl;
    return new;
}

size_t vx_IrBlock_insert_label_op(vx_IrBlock *block) {
    vx_IrOp *label_decl = vx_IrBlock_add_op_building(block);
    size_t label_id = vx_IrBlock_new_label(block, label_decl);
    vx_IrOp_init(label_decl, VX_LIR_OP_LABEL, block);
    vx_IrOp_add_param_s(label_decl, VX_IR_NAME_ID, (vx_IrValue) { .type = VX_IR_VAL_ID, .id = label_id });
    return label_id;
}

/** false for nop and label   true for everything else */
bool vx_IrOpType_has_effect(vx_IrOpType type) {
    switch (type) {
    case VX_IR_OP_NOP:
    case VX_LIR_OP_LABEL:
        return false;

    default:
        return true;
    }
}

