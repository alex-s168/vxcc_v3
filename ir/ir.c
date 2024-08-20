#include "ir.h"
#include "cir.h"
#include "llir.h"

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

void vx_IrBlock_llir_compact(vx_IrBlock *root) {
    assert(root->is_root);

    for (vx_IrVar idx = 0; idx < root->as_root.vars_len; idx ++) {
        if (root->as_root.vars[idx].ll_type == NULL) {
            if (idx + 1 >= root->as_root.vars_len) break;
            for (vx_IrVar after = idx + 1; after < root->as_root.vars_len; after ++) {
                assert(root != NULL && root->is_root);
                vx_IrBlock_rename_var(root, after, after - 1);
            }
        }
    }
}

// TODO: remove cir checks and make sure fn called after cir type expand & MAKE TYPE EXPAND AWARE OF MEMBER ALIGN FOR UNIONS
size_t vx_IrType_size(vx_IrType *ty) {
    assert(ty != NULL);

    size_t total = 0; 

    switch (ty->kind) {
    case VX_IR_TYPE_KIND_BASE:
        return ty->base.size;

    case VX_IR_TYPE_KIND_CIR_UNION:
        for (size_t i = 0; i < ty->cir_union.members_len; i ++) {
            size_t val = vx_IrType_size(ty->cir_union.members[i]);
            if (val > total)
                total = val;
        }
        return total;

    case VX_IR_TYPE_KIND_CIR_STRUCT:
        for (size_t i = 0; i < ty->cir_struct.members_len; i ++) {
            total += vx_IrType_size(ty->cir_struct.members[i]);
        }
        return total;

    case VX_IR_TYPE_FUNC:
        return PTRSIZE;

    default:
	fprintf(stderr, "type %u doesn't have size\n", ty->kind);
	fflush(stderr);
	exit(1);
	assert(false);
	return 0;
    }
}

void vx_IrType_free(vx_IrType *ty) {
    switch (ty->kind) {
    case VX_IR_TYPE_KIND_BASE:
        return;

    case VX_IR_TYPE_KIND_CIR_UNION:
        free(ty->cir_union.members);
        return;

    case VX_IR_TYPE_KIND_CIR_STRUCT:
        free(ty->cir_struct.members);
        return;

    case VX_IR_TYPE_FUNC:
        free(ty->func.args);
        return;

    default:
	assert(false);
	return;
    }
}

vx_IrTypeRef vx_IrValue_type(vx_IrBlock* root, vx_IrValue value) {
    switch (value.type) {
        case VX_IR_VAL_IMM_INT:
        case VX_IR_VAL_IMM_FLT:
        case VX_IR_VAL_UNINIT:
            return (vx_IrTypeRef) { .ptr = value.no_read_rt_type, .shouldFree = false };

        case VX_IR_VAL_ID:
            return vx_ptrtype(root);

	default:
	    assert(false);
        case VX_IR_VAL_TYPE:
        case VX_IR_VAL_BLOCK:
            return (vx_IrTypeRef) { .ptr = NULL, .shouldFree = false };

        case VX_IR_VAL_BLOCKREF:
            return vx_IrBlock_type(value.block);

        case VX_IR_VAL_VAR:
            return (vx_IrTypeRef) { .ptr = vx_IrBlock_typeof_var(root, value.var), .shouldFree = false };
    }
}

/*
typedef struct {
    vx_IrVar var;
    vx_IrOp* decl;
} vx_CIrBlock_fix_state;

bool vx_CIrBlock_fix_iter(vx_IrOp* op, void* param) {
    vx_CIrBlock_fix_state* state = param;

    for (size_t i = 0; i < op->outs_len; i ++) {
        if (op->outs[i].var == state->var) {
            state->decl = op;
            return true;
        }
    }
    return false;
}*/

void vx_CIrBlock_fix(vx_IrBlock* block) {
    vx_IrBlock* root = vx_IrBlock_root(block);

    for (size_t i = 0; i < block->ins_len; i ++)
        vx_IrBlock_putVar(root, block->ins[i].var, NULL);

    for (vx_IrOp* op = block->first; op; op = op->next) {
        for (size_t i = 0; i < op->outs_len; i ++)
            vx_IrBlock_putVar(root, op->outs[i].var, op);

        for (size_t i = 0; i < op->params_len; i ++)
            if (op->params[i].val.type == VX_IR_VAL_BLOCK)
                vx_CIrBlock_fix(op->params[i].val.block);
    }
}

/*void vx_CIrBlock_fix(vx_IrBlock *root) {
    assert(root->is_root);

    for (vx_IrVar var = 0; var < root->as_root.vars_len; var ++) {
        vx_CIrBlock_fix_state state;
        state.var = var;
        state.decl = NULL;

        vx_IrBlock_deep_traverse(root, vx_CIrBlock_fix_iter, &state);

        root->as_root.vars[var].decl = state.decl;
    }
}*/

void vx_IrBlock_llir_fix_decl(vx_IrBlock *root) {
    assert(root->is_root);

    // TODO: WHY THE FUCK IS THIS EVEN REQUIRED????

    memset(root->as_root.labels, 0, sizeof(*root->as_root.labels) * root->as_root.labels_len);
    memset(root->as_root.vars, 0, sizeof(*root->as_root.vars) * root->as_root.vars_len);

    for (size_t i = 0; i < root->ins_len; i ++) {
        vx_IrVar v = root->ins[i].var;
        assert(v < root->as_root.vars_len);
        root->as_root.vars[v].ll_type = root->ins[i].type;
    }

    for (vx_IrOp *op = root->first; op; op = op->next) {
        for (size_t j = 0; j < op->outs_len; j ++) {
            vx_IrTypedVar out = op->outs[j];
            assert(out.var < root->as_root.vars_len);
            vx_IrOp **decl = &root->as_root.vars[out.var].decl;
            if (*decl == NULL) {
                *decl = op;
                root->as_root.vars[out.var].ll_type = out.type;
            }
        }

        if (op->id == VX_LIR_OP_LABEL) {
            size_t id = vx_IrOp_param(op, VX_IR_NAME_ID)->id;
            assert(id < root->as_root.labels_len);
            vx_IrOp **decl = &root->as_root.labels[id].decl;
            if (*decl == NULL) {
                *decl = op;
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
    while (true) {
        if (block == NULL) break;
        if (block->is_root) break;
        if (block->parent == NULL) break; // forgot to mark is_root
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
    assert(root->as_root.vars);
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
    assert(root->as_root.labels);
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

vx_IrTypeRef vx_IrBlock_type(vx_IrBlock* block) {
    // TODO: multiple rets
    vx_IrType *ret = block->outs_len == 1 ? vx_IrBlock_typeof_var(block, block->outs[0])
                                          : NULL;

    vx_IrType **args = malloc(sizeof(vx_IrType*) * block->ins_len);
    assert(args);
    for (size_t i = 0; i < block->ins_len; i ++) {
        args[i] = block->ins[i].type;
    }

    vx_IrType *type = malloc(sizeof(vx_IrType));
    assert(type);
    type->kind = VX_IR_TYPE_FUNC;
    type->func.nullableReturnType = ret;
    type->func.args_len = block->ins_len;
    type->func.args = args;

    return (vx_IrTypeRef) { .ptr = type, .shouldFree = true };
}
