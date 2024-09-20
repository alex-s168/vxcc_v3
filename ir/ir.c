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
                vx_IrBlock_renameVar(root, after, after - 1, VX_RENAME_VAR_BOTH);
            }
        }
    }
}

// TODO: remove cir checks and make sure fn called after cir type expand

size_t vx_IrType_size(vx_IrType *ty) {
    assert(ty != NULL);

    size_t total = 0; 

    switch (ty->kind) {
    case VX_IR_TYPE_KIND_BASE:
        return ty->base.size;

    case VX_IR_TYPE_KIND_CIR_STRUCT:
        for (size_t i = 0; i < ty->cir_struct.members_len; i ++) {
            total += vx_IrType_size(ty->cir_struct.members[i]);
        }
        return total;

    case VX_IR_TYPE_FUNC:
        return 8; // TODO

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
            return vx_ptrType(root);

	default:
	    assert(false);
        case VX_IR_VAL_TYPE:
        case VX_IR_VAL_BLOCK:
            return (vx_IrTypeRef) { .ptr = NULL, .shouldFree = false };

        case VX_IR_VAL_BLOCKREF:
            return vx_IrBlock_type(value.block);

        case VX_IR_VAL_VAR:
            return (vx_IrTypeRef) { .ptr = vx_IrBlock_typeofVar(root, value.var), .shouldFree = false };
    }
}

void vx_CIrBlock_fix(vx_IrBlock* block) {
    vx_IrBlock* root = vx_IrBlock_root(block);

    for (size_t i = 0; i < block->ins_len; i ++)
        vx_IrBlock_putVar(root, block->ins[i].var, NULL);

    for (vx_IrOp* op = block->first; op; op = op->next) {
        for (size_t i = 0; i < op->outs_len; i ++)
            vx_IrBlock_putVar(root, op->outs[i].var, op);

        FOR_INPUTS(op, inp, {
           if (inp.type == VX_IR_VAL_BLOCK)
                vx_CIrBlock_fix(inp.block);
        });
    }
}

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
            if (vx_IrType_size(out.type) == 0) {
                fprintf(stderr, "size of type %s in 0", out.type->debugName);
                exit(1);
            }
            assert(out.var < root->as_root.vars_len);
            vx_IrOp **decl = &root->as_root.vars[out.var].decl;
            if (*decl == NULL) {
                *decl = op;
                root->as_root.vars[out.var].ll_type = out.type;
            }
        }

        if (op->id == VX_IR_OP_LABEL) {
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

bool vx_IrBlock_deepTraverse(vx_IrBlock *block, bool (*callback)(vx_IrOp *op, void *data), void *data) {
    for (vx_IrOp *op = block->first; op; op = op->next) {
        FOR_INPUTS(op, inp, {
            if (inp.type == VX_IR_VAL_BLOCK)
                if (vx_IrBlock_deepTraverse(inp.block, callback, data))
                    return true;
        });

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

void vx_IrBlock_swapInAt(vx_IrBlock *block, const size_t a, const size_t b) {
    if (a == b)
        return;

    const vx_IrVar va = block->ins[a].var;
    block->ins[a] = block->ins[b];
    block->ins[b].var = va;
}

void vx_IrBlock_swapOutAt(vx_IrBlock *block, size_t a, size_t b) {
    if (a == b)
        return;

    const vx_IrVar va = block->outs[a];
    block->outs[a] = block->outs[b];
    block->outs[b] = va;
}

vx_IrVar vx_IrBlock_newVar(vx_IrBlock *block, vx_IrOp *decl) {
    assert(block != NULL);
    vx_IrBlock *root = (vx_IrBlock *) vx_IrBlock_root(block);
    assert(root != NULL);
    root->as_root.vars = realloc(root->as_root.vars, (root->as_root.vars_len + 1) * sizeof(*root->as_root.vars));
    assert(root->as_root.vars);
    vx_IrVar new = root->as_root.vars_len ++;
    memset(&root->as_root.vars[new], 0, sizeof(*root->as_root.vars));
    root->as_root.vars[new].decl = decl;
    return new;
}

size_t vx_IrBlock_newLabel(vx_IrBlock *block, vx_IrOp *decl) {
    assert(block != NULL);
    vx_IrBlock *root = (vx_IrBlock *) vx_IrBlock_root(block);
    assert(root != NULL);
    root->as_root.labels = realloc(root->as_root.labels, (root->as_root.labels_len + 1) * sizeof(*root->as_root.labels));
    assert(root->as_root.labels);
    size_t new = root->as_root.labels_len ++;
    root->as_root.vars[new].decl = decl;
    return new;
}

size_t vx_IrBlock_appendLabelOp(vx_IrBlock *block) {
    vx_IrOp *label_decl = vx_IrBlock_addOpBuilding(block);
    vx_IrOp_init(label_decl, VX_IR_OP_LABEL, block);
    size_t label_id = vx_IrBlock_newLabel(block, label_decl);
    vx_IrOp_addParam_s(label_decl, VX_IR_NAME_ID, VX_IR_VALUE_ID(label_id));
    return label_id;
}

void vx_IrBlock_appendLabelOpPredefined(vx_IrBlock *block, size_t label_id) {
    vx_IrOp *label_decl = vx_IrBlock_addOpBuilding(block);
    vx_IrOp_init(label_decl, VX_IR_OP_LABEL, block);
    vx_IrOp_addParam_s(label_decl, VX_IR_NAME_ID, VX_IR_VALUE_ID(label_id));

    vx_IrBlock* root = vx_IrBlock_root(block);
    root->as_root.labels[label_id].decl = label_decl;
}

vx_IrTypeRef vx_IrBlock_type(vx_IrBlock* block) {
    // TODO: multiple rets
    vx_IrType *ret = block->outs_len == 1 ? vx_IrBlock_typeofVar(block, block->outs[0])
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

#include "opt.h"
#include "../cg/x86_stupid/cg.h"

/** 0 if ok */
int vx_CU_compile(vx_CU * cu,
                  FILE* optionalOptimizedSsaIr,
                  FILE* optionalOptimizedLlIr,
                  FILE* optionalAsm,
                  vx_BinFormat optionalBinFormat, FILE* optionalBinOut)
{

#define FOR_BLOCK(code) \
    for (size_t i = 0; i < cu->blocks_len; i ++) { \
        if (cu->blocks[i].type == VX_CU_BLOCK_IR) { vx_IrBlock* block = cu->blocks[i].v.ir; code; } \
    }

    FOR_BLOCK({
        if (optionalOptimizedSsaIr != NULL)
            vx_IrBlock_dump(block, optionalOptimizedSsaIr, 0);
    });

    FOR_BLOCK({
        vx_CIrBlock_fix(block); // TODO: why...
        vx_CIrBlock_normalize(block);
        vx_IrBlock_flattenFlattenPleaseRec(block);
        vx_CIrBlock_mksa_states(block);
        vx_CIrBlock_mksa_final(block);
        vx_CIrBlock_fix(block); // TODO: why...

        puts("post CIR lower:");
        vx_IrBlock_dump(block, stdout, 0);

        if (vx_ir_verify(block) != 0)
            return 1;
    });

    FOR_BLOCK({
        opt(block);

        puts("post SSA IR opt:");
        vx_IrBlock_dump(block, stdout, 0); // TODO remove 

        vx_IrBlock_flattenFlattenPleaseRec(block);

        if (optionalOptimizedSsaIr != NULL)
            vx_IrBlock_dump(block, optionalOptimizedSsaIr, 0);

        if (vx_ir_verify(block) != 0)
            return 1;
    });

    FOR_BLOCK({
        vx_IrBlock_llir_preLower_loops(block);
        vx_IrBlock_llir_lower(block);
        vx_IrBlock_llir_fix_decl(block);
    });

    FOR_BLOCK({
        opt_ll(block);

        if (optionalOptimizedLlIr != NULL)
            vx_IrBlock_dump(block, optionalOptimizedLlIr, 0);

        llir_prep_lower(block);

        if (optionalAsm)
            vx_cg_x86stupid_gen(block, optionalAsm);
    });

#undef FOR_BLOCK

    return 0;
}
