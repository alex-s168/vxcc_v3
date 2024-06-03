#include "ir.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>



vx_IrOp *vx_IrBlock_tail(vx_IrBlock *block) {
    vx_IrOp *op = block->first;
    while (op && op->next) {
        op = op->next;
    }
    return op;
}

vx_RegRefList vx_RegRefList_fixed(size_t count) {
    vx_RegRefList list;
    list.count = count;
    list.items = fastalloc(sizeof(vx_RegRef) * count);
    return list;
}

bool vx_RegRefList_contains(vx_RegRefList list, vx_RegRef reg) {
    for (size_t i = 0; i < list.count; i ++)
        if (list.items[i] == reg)
            return true;
    return false;
}

vx_RegRefList vx_RegRefList_intersect(vx_RegRefList a, vx_RegRefList b) {
    vx_RegRefList res = vx_RegRefList_fixed(a.count);
    res.count = 0;

    for (size_t i = 0; i < a.count; i ++) {
        vx_RegRef reg = a.items[i];
        if (vx_RegRefList_contains(b, reg)) {
            res.items[res.count ++] = reg;
        }
    }

    return res;
}

vx_RegRefList vx_RegRefList_union(vx_RegRefList a, vx_RegRefList b) {
    vx_RegRefList res = vx_RegRefList_fixed(a.count + b.count);

    for (size_t i = 0; i < a.count; i ++)
        res.items[i] = a.items[i];

    for (size_t i = 0; i < b.count; i ++)
        res.items[i + a.count] = b.items[i];

    return res;
}

vx_RegRefList vx_RegRefList_remove(vx_RegRefList a, vx_RegRefList rem) {
    vx_RegRefList res = vx_RegRefList_fixed(a.count);
    res.count = 0;

    for (size_t i = 0; i < a.count; i ++) {
        vx_RegRef ref = a.items[i];
        if (!vx_RegRefList_contains(rem, ref)) {
            res.items[res.count ++] = ref;
        }
    }

    return res;
}

bool vx_RegAllocConstraint_matches(vx_RegAllocConstraint constraint, vx_RegRef reg) {
    switch (constraint.kind) {
    case VX_SEL_ANY:
        return true;

    case VX_SEL_NONE:
        return false;

    case VX_SEL_ONE_OF:
        return vx_RegRefList_contains(constraint.value, reg);

    case VX_SEL_NONE_OF:
        return !vx_RegRefList_contains(constraint.value, reg);
    }
}

static vx_RegAllocConstraint merge__(vx_RegAllocConstraint a, vx_RegAllocConstraint b) {
    if (a.kind == VX_SEL_NONE || b.kind == VX_SEL_NONE)
        return a;

    if (a.kind == VX_SEL_ANY)
        return b;

    if (b.kind == VX_SEL_ANY)
        return a;

    if (a.kind == VX_SEL_ONE_OF && b.kind == VX_SEL_ONE_OF) {
        a.value = vx_RegRefList_intersect(a.value, b.value);
        return a;
    }

    if (a.kind == VX_SEL_NONE_OF && b.kind == VX_SEL_NONE_OF) {
        a.value = vx_RegRefList_union(a.value, b.value);
        return a;
    }

    if (a.kind == VX_SEL_NONE_OF) {
        vx_RegAllocConstraint temp = a;
        a = b;
        b = temp;
    }

    assert(a.kind == VX_SEL_ONE_OF);
    assert(b.kind == VX_SEL_NONE_OF);

    a.value = vx_RegRefList_remove(a.value, b.value);
    return a;
}

vx_RegAllocConstraint vx_RegAllocConstraint_merge(vx_RegAllocConstraint a, vx_RegAllocConstraint b) {
    vx_RegAllocConstraint res = merge__(a, b);
    res.or_mem = a.or_mem && b.or_mem;
    res.or_stack = a.or_stack && b.or_stack;
    return res;
}

void vx_IrBlock_llir_fix_decl(vx_IrBlock *root) {
    assert(root->is_root);

    // TODO: WHY THE FUCK IS THIS EVEN REQUIRED????

    memset(root->as_root.vars, 0, sizeof(*root->as_root.vars) * root->as_root.vars_len);

    size_t total = 0; 
    for (vx_IrOp *op = root->first; op; op = op->next) {
        for (size_t j = 0; j < op->outs_len; j ++) {
            vx_IrTypedVar out = op->outs[j];
            vx_IrOp **decl = &root->as_root.vars[out.var].decl;
            if (*decl == NULL) {
                *decl = op;
                root->as_root.vars[out.var].ll_type = out.type;
                total ++;
            }
        }
    }
}

void vx_IrOp_warn(vx_IrOp *op, const char *optMsg0, const char *optMsg1) {
    vx_IrBlock *parent = op ? op->parent : NULL;
    if (parent) {
        fprintf(stderr, "WARN: %s %s\n", optMsg0 ? optMsg0 : "", optMsg1 ? optMsg1 : "");
    } else {
        fprintf(stderr, "WARN: trying to warn() for non existant op!\n");
    }
}

bool vx_IrBlock_deep_traverse(vx_IrBlock *block, bool (*callback)(vx_IrOp *op, void *data), void *data) {
    for (vx_IrOp *op = block->first; op; op = op->next) {
        for (size_t j = 0; j < op->params_len; j ++)
            if (op->params[j].val.type == VX_IR_VAL_BLOCK)
                if (vx_IrBlock_deep_traverse(op->params[j].val.block, callback, data))
                    return true;

        if (callback(op, data))
            return true;
    }
    return false;
}

vx_IrBlock *vx_IrBlock_root(vx_IrBlock *block) {
    while (block != NULL && !block->is_root) {
        block = block->parent;
    }
    return block;
}

void vx_IrBlock_swap_in_at(vx_IrBlock *block, const size_t a, const size_t b) {
    if (a == b)
        return;

    const vx_IrVar va = block->ins[a].var;
    block->ins[a] = block->ins[b];
    block->ins[b].var = va;
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
    root->as_root.vars[new].decl = decl;
    return new;
}

size_t vx_IrBlock_append_label_op(vx_IrBlock *block) {
    vx_IrOp *label_decl = vx_IrBlock_add_op_building(block);
    vx_IrOp_init(label_decl, VX_LIR_OP_LABEL, block);
    size_t label_id = vx_IrBlock_new_label(block, label_decl);
    vx_IrOp_add_param_s(label_decl, VX_IR_NAME_ID, (vx_IrValue) { .type = VX_IR_VAL_ID, .id = label_id });
    return label_id;
}

/** false for nop and label   true for everything else */
bool vx_IrOpType_has_effect(vx_IrOpType type) {
    switch (type) {
    case VX_LIR_OP_LABEL:
        return false;

    default:
        return true;
    }
}

