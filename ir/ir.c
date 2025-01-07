#include "ir.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void vx_IrBlock_markVarOrigin(vx_IrBlock* block, vx_IrVar old, vx_IrVar new)
{
	vx_IrBlock* root = vx_IrBlock_root(block);
	assert(root && root->is_root);

	root->as_root.vars[new].heat = root->as_root.vars[old].heat;
	root->as_root.vars[new].ll_lifetime = root->as_root.vars[old].ll_lifetime;
}

vx_IrName vx_IrName_parse(const char * src, uint32_t srcLen)
{
    for (vx_IrName i = 0; i < VX_IR_NAME__LAST; i ++)
    {
        const char * nam = vx_IrName_str[i];
        uint32_t nam_len = strlen(nam);
        if (nam_len != srcLen) continue;
        if (!memcmp(nam, src, nam_len)) {
            return i;
        }
    }
    assert(false && "could not parse ir instr param name");
    return VX_IR_NAME__LAST; // unreachable
}

bool vx_IrOpType_parse(vx_IrOpType* dest, const char * name, size_t name_len)
{
    for (uint32_t i = 0; i < vx_IrOpType__len; i ++)
    {
        const char *cmp = vx_IrOpType__entries[i].debug.a;
        if (strlen(cmp) != name_len) continue;
        if (memcmp(cmp, name, name_len) == 0) {
            *dest = i;
            return true;
        }
    }
    return false;
}

void vx_CU_init(vx_CU* dest, const char * targetStr)
{
    memset(dest, 0, sizeof(vx_CU));

    vx_Target_parse(&dest->target, targetStr);
    vx_Target_info(&dest->info, &dest->target);

    dest->opt = (vx_OptConfig) {
        .max_total_cmov_inline_cost = 4,
        .consteval_iterations = 6,
        .loop_simplify = true,
        .if_eval = true,
    };
}

bool vx_IrValue_eq(vx_IrValue a, vx_IrValue b)
{
    if (a.type != b.type)
        return false;

    switch (a.type)
    {
        case VX_IR_VAL_IMM_INT:
            return a.imm_int == b.imm_int;

        case VX_IR_VAL_IMM_FLT:
            return a.imm_flt == b.imm_flt;

        case VX_IR_VAL_VAR:
            return a.var == b.var;

        case VX_IR_VAL_UNINIT:
            return true;

        case VX_IR_VAL_BLOCKREF:
            return a.block == b.block;

        case VX_IR_VAL_BLOCK:
            return a.block == b.block;

        case VX_IR_VAL_TYPE:
            return a.ty == b.ty;

        case VX_IR_VAL_ID:
            return a.id == b.id;

		case VX_IR_VAL_X86_CC:
			return !strcmp(a.x86_cc, b.x86_cc);

		case VX_IR_VAL_SYMREF:
			return !strcmp(a.symref, b.symref);
    }
}

vx_IrOp *vx_IrBlock_tail(vx_IrBlock *block) {
    vx_IrOp *op = block->first;
    while (op && op->next) {
        op = op->next;
    }
    return op;
}

// TODO: remove cir checks and make sure fn called after cir type expand

size_t vx_IrType_size(vx_CU* cu, vx_IrBlock* inCtx, vx_IrType *ty) {
    assert(ty != NULL);

    size_t total = 0; 

    switch (ty->kind) {
    case VX_IR_TYPE_KIND_BASE:
        return ty->base.size;

    case VX_IR_TYPE_FUNC:
        return vx_IrType_size(cu, inCtx, cu->info.get_ptr_ty(cu, inCtx));

    default:
		fprintf(stderr, "type %u doesn't have size\n", ty->kind);
		fflush(stderr);
		exit(1);
		assert(false);
		return 0;
    }
}

vx_IrType* vx_IrValue_type(vx_CU* cu, vx_IrBlock* root, vx_IrValue value) {
    switch (value.type) 
	{
        case VX_IR_VAL_IMM_INT:
        case VX_IR_VAL_IMM_FLT:
        case VX_IR_VAL_UNINIT:
            return value.no_read_rt_type;

        case VX_IR_VAL_ID:
            return cu->info.get_ptr_ty(cu, root);

		case VX_IR_VAL_TYPE:
        case VX_IR_VAL_BLOCK:
            return NULL;

        case VX_IR_VAL_BLOCKREF:
            return vx_IrBlock_type(value.block);

        case VX_IR_VAL_VAR:
            return vx_IrBlock_typeofVar(root, value.var);

		default:
			assert(false);
			return NULL;
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

vx_IrType* vx_IrBlock_type(vx_IrBlock* block) {
    vx_IrType *type = fastalloc(sizeof(vx_IrType));
    assert(type);
    type->kind = VX_IR_TYPE_FUNC;

	vx_IrType **args = fastalloc(sizeof(vx_IrType*) * block->ins_len);
    assert(args);
    for (size_t i = 0; i < block->ins_len; i ++)
        args[i] = block->ins[i].type;
    type->func.args_len = block->ins_len;
    type->func.args = args;

	vx_IrType **rets = fastalloc(sizeof(vx_IrType*) * block->outs_len);
	assert(rets);
	for (size_t i = 0; i < block->outs_len; i ++)
		rets[i] = vx_IrBlock_typeofVar(block, block->outs[i]);
	type->func.rets_len = block->outs_len;
	type->func.rets = rets;

    return type;
}

void vx_IrOp_updateParent(vx_IrOp* op, vx_IrBlock* to)
{
    op->parent = to;
    FOR_INPUTS(op, inp, ({
        if (inp.type == VX_IR_VAL_BLOCK)
            inp.block->parent = to;
    }));
}

#include "passes.h"

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
		if (!block->is_root) {
			fprintf(stderr, "only root blocks allowed in CU\n");
			exit(1);
		}

        vx_CIrBlock_fix(cu, block); // TODO: why...
        vx_CIrBlock_normalize(cu, block);
        vx_CIrBlock_mksa_states(cu, block);
        vx_CIrBlock_mksa_final(cu, block);
        vx_CIrBlock_fix(cu, block); // TODO: why...

        if (vx_ir_verify(cu, block) != 0)
            return 1;
    });

    FOR_BLOCK({
        vx_opt(cu, block);

        if (optionalOptimizedSsaIr != NULL)
            vx_IrBlock_dump(block, optionalOptimizedSsaIr, 0);

        if (vx_ir_verify(cu, block) != 0)
            return 1;
    });

    FOR_BLOCK({
		vx_IrBlock_root_varsHeat(block);

        vx_IrBlock_llir_preLower_loops(cu, block);
        vx_IrBlock_llir_preLower_ifs(cu, block);
        vx_opt_preLower(cu, block);
        vx_IrBlock_llir_lower(cu, block);
        vx_IrBlock_llir_fix_decl(cu, block);
    });

    FOR_BLOCK({
        vx_opt_ll(cu, block);

        if (optionalOptimizedLlIr != NULL)
            vx_IrBlock_dump(block, optionalOptimizedLlIr, 0);

        vx_llir_prep_lower(cu, block);
		vx_IrBlock_llir_varsHeat(block);

        vx_IrBlock_dump(block, stdout, 0);
    });

    if (optionalAsm) {
        for (size_t i = 0; i < cu->blocks_len; i ++) {
            vx_CUBlock* cb = &cu->blocks[i];
            switch (cb->type) {
                case VX_CU_BLOCK_IR:
                {
					fprintf(optionalAsm, "section .text\n");
                    if (cb->do_export)
                        fprintf(optionalAsm, "    global %s\n", cb->v.ir->name);
					vx_llir_emit_asm(cu, cb->v.ir, optionalAsm);
                    break;
                }

                case VX_CU_BLOCK_BLK_REF:
                {
					fprintf(optionalAsm, "section .text\n");
                    fprintf(optionalAsm, "    extern %s\n", cb->v.blk_ref->name);
                    break;
                }
				
				case VX_CU_BLOCK_ASM:
				{
					fprintf(optionalAsm, "section .text\n");
					fprintf(optionalAsm, "; generated by frontend\n%s\n", cb->v.asmb);
				}

				case VX_CU_BLOCK_DATA:
				{
					fprintf(optionalAsm, "section .data\n");
                    if (cb->do_export)
                        fprintf(optionalAsm, "    global %s\n", cb->v.data->name);
					fprintf(optionalAsm, "%s: ; <%zu * %s>\n", cb->v.data->name,
															   cb->v.data->numel,
															   cb->v.data->elty->debugName);
					fprintf(optionalAsm, "  db ");
					for (size_t i = 0; i < cb->v.data->comptime_elt_size * cb->v.data->numel; i ++)
						fprintf(optionalAsm, "%i, ", ((char *) cb->v.data->data)[i]);
					fprintf(optionalAsm, "\n\n");
				}
            }
        }
    }

#undef FOR_BLOCK

    return 0;
}
