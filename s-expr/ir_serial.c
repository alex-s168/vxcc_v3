#include "../ir/ir.h"
#include "sexpr.h"

vx_IrVar vx_IrVar_parseS(vx_CU* cu, struct SNode* nd)
{
	snode_expect(nd, S_INTEGER);
	vx_IrVar out;
	sscanf(nd->value, "%zu", &out);
	return out;
}

vx_IrTypedVar vx_IrTypedVar_parseS(vx_CU* cu, struct SNode* nd)
{
	nd = snode_expect(nd, S_LIST)->list;

	struct SNode* s0 = snode_geti_expect(nd, 0);
	snode_expect(s0, S_STRING);
	vx_IrType* ty = vx_CU_typeByName(cu, s0->value);
	if (!ty) {
		fprintf(stderr, "type %s not registered\n", s0->value);
		exit(1);
	}

	vx_IrVar var = vx_IrVar_parseS(cu, snode_geti_expect(nd, 1));

	return (vx_IrTypedVar) {
		.type = ty,
		.var = var,
	};
}

vx_IrBlock* vx_IrBlock_parseS(vx_CU* cu, struct SNode* nd);

vx_IrValue vx_IrValue_parseS(vx_CU* cu, struct SNode* nd)
{
	nd = snode_expect(nd, S_LIST)->list;

	struct SNode* kind = snode_geti_expect(nd, 0);
	snode_expect(kind, S_SYMBOL);

	if (!strcmp(kind->value, "int")) {
		struct SNode* v = snode_geti_expect(nd, 1);
		snode_expect(v, S_INTEGER);
		long long value;
		sscanf(v->value, "%lli", &value);
		return VX_IR_VALUE_IMM_INT(value);
	}
	else if (!strcmp(kind->value, "float")) {
		struct SNode* v = snode_geti_expect(nd, 1);
		snode_expect(v, S_FLOAT);
		double value;
		sscanf(v->value, "%lf", &value);
		return VX_IR_VALUE_IMM_FLT(value);
	}
	else if (!strcmp(kind->value, "var")) {
		struct SNode* v = snode_geti_expect(nd, 1);
		return VX_IR_VALUE_VAR(vx_IrVar_parseS(cu, v));
	}
	else if (!strcmp(kind->value, "uninit")) {
		return VX_IR_VALUE_UNINIT();
	}
	else if (!strcmp(kind->value, "label")) {
		struct SNode* v = snode_geti_expect(nd, 1);
		snode_expect(v, S_INTEGER);
		size_t value;
		sscanf(v->value, "%zu", &value);
		return VX_IR_VALUE_ID(value);
	}
	else if (!strcmp(kind->value, "x86-cc")) {
		struct SNode* v = snode_geti_expect(nd, 1);
		snode_expect(v, S_STRING);
		return VX_IR_VALUE_X86_CC(v->value);
	}
	else if (!strcmp(kind->value, "type")) {
		struct SNode* v = snode_geti_expect(nd, 1);
		snode_expect(v, S_STRING);
		vx_IrType* ty = vx_CU_typeByName(cu, v->value);
		if (!ty) {
			fprintf(stderr, "type %s not registered\n", v->value);
			exit(1);
		}
		return VX_IR_VALUE_TYPE(ty);
	}
	else if (!strcmp(kind->value, "blockref")) {
		struct SNode* v = snode_geti_expect(nd, 1);
		snode_expect(v, S_STRING);
		vx_IrBlock* b = vx_CU_blockByName(cu, v->value);
		if (!b) {
			fprintf(stderr, "named block %s not registered\n", v->value);
			exit(1);
		}
		return VX_IR_VALUE_BLKREF(b);
	}
	else if (!strcmp(kind->value, "symref")) {
		struct SNode* v = snode_geti_expect(nd, 1);
		snode_expect(v, S_STRING);
		return VX_IR_VALUE_SYMREF(faststrdup(v->value));
	}
	else if (!strcmp(kind->value, "block")) {
		struct SNode* v = snode_geti_expect(nd, 1);
		return VX_IR_VALUE_BLK(vx_IrBlock_parseS(cu, v));
	}
	else {
		fprintf(stderr, "invalid value type\n");
		exit(1);
	}
}

vx_IrOp* vx_IrOp_parseS(vx_CU* cu, struct SNode* s)
{
	s = snode_expect(s, S_LIST)->list;

	struct SNode* outs = snode_expect(snode_kv_get_expect(s, "outs"), S_LIST)->list;
	struct SNode* params = snode_expect(snode_kv_get_expect(s, "params"), S_LIST)->list;
	struct SNode* args = snode_expect(snode_kv_get_expect(s, "args"), S_LIST)->list;
	char const * name = snode_expect(snode_kv_get_expect(s, "name"), S_STRING)->value;

	vx_IrOpType opty;
	if (!vx_IrOpType_parsec(&opty, name)) {
		fprintf(stderr, "there is no operation with the name \"%s\"\n", name);
		exit(1);
	}

	vx_IrOp* op = fastalloc(sizeof(vx_IrOp));
	vx_IrOp_init(op, opty, NULL);

	for (; outs; outs = outs->next) {
		vx_IrTypedVar out = vx_IrTypedVar_parseS(cu, outs);
		vx_IrOp_addOut(op, out.var, out.type);
	}

	for (; params; params = params->next) {
		struct SNode* pair = snode_expect(params, S_LIST)->list;
		vx_IrName pair_name = vx_IrName_parsec(snode_expect(snode_geti_expect(pair, 0), S_STRING)->value);
		vx_IrValue pair_value = vx_IrValue_parseS(cu, snode_geti_expect(pair, 1));
		vx_IrOp_addParam_s(op, pair_name, pair_value);
	}

	for (; args; args = args->next) {
		vx_IrValue v = vx_IrValue_parseS(cu, args);
		vx_IrOp_addArg(op, v);
	}

	return op;
}

vx_IrBlock* vx_IrBlock_parseS(vx_CU* cu, struct SNode* s)
{
	s = snode_expect(s, S_LIST)->list;

	struct SNode* args = snode_expect(snode_kv_get_expect(s, "args"), S_LIST)->list;
	struct SNode* ops = snode_expect(snode_kv_get_expect(s, "ops"), S_LIST)->list;
	struct SNode* rets = snode_expect(snode_kv_get_expect(s, "rets"), S_LIST)->list;

	vx_IrBlock* block = vx_IrBlock_initHeap(NULL, NULL);

	for (; args; args = args->next) {
		vx_IrTypedVar arg = vx_IrTypedVar_parseS(cu, args);
		vx_IrBlock_addIn(block, arg.var, arg.type);
	}

	for (; ops; ops = ops->next) {
		vx_IrOp* op = vx_IrOp_parseS(cu, ops);
		op->parent = block;
		vx_IrBlock_addOp(block, op);
	}

	for (; rets; rets = rets->next) {
		vx_IrVar ret = vx_IrVar_parseS(cu, rets);
		vx_IrBlock_addOut(block, ret);
	}

	return block;
}

struct SNode* vx_IrVar_emitS(vx_CU* cu, vx_IrVar v)
{
	char buf[64];
	sprintf(buf, "%zu", v);
	return snode_mk(S_INTEGER, buf);
}


struct SNode* vx_IrTypedVar_emitS(vx_CU* cu, vx_IrTypedVar tv)
{
	struct SNode* name = snode_mk(S_STRING, tv.type->debugName);
	name->next = vx_IrVar_emitS(cu, tv.var);
	return snode_mk_list(name);
}

struct SNode* vx_IrBlock_emitS(vx_CU* cu, vx_IrBlock* block);

struct SNode* vx_IrValue_emitS(vx_CU* cu, vx_IrValue val)
{
	char const * names[] = {
		[VX_IR_VAL_IMM_INT] = "int",
		[VX_IR_VAL_IMM_FLT] = "float",
		[VX_IR_VAL_VAR] = "var",
		[VX_IR_VAL_UNINIT] = "uninit",
		[VX_IR_VAL_ID] = "label",
		[VX_IR_VAL_X86_CC] = "x86-cc",
		[VX_IR_VAL_TYPE] = "type",
		[VX_IR_VAL_BLOCKREF] = "blockref",
		[VX_IR_VAL_SYMREF] = "symref",
		[VX_IR_VAL_BLOCK]= "block",
	};

	struct SNode* nd = snode_mk(S_SYMBOL, names[val.type]);

	switch (val.type) {
		case VX_IR_VAL_IMM_INT: {
			char buf[64];
			sprintf(buf, "%lli", val.imm_int);
			nd->next = snode_mk(S_INTEGER, buf);
			break;
		}

		case VX_IR_VAL_IMM_FLT:{
			char buf[64];
			sprintf(buf, "%lf", val.imm_flt);
			nd->next = snode_mk(S_FLOAT, buf);
			break;
		}

		case VX_IR_VAL_VAR: {
			nd->next = vx_IrVar_emitS(cu, val.var);
			break;
		}

		case VX_IR_VAL_UNINIT:
			break;

		case VX_IR_VAL_ID: {
			char buf[64];
			sprintf(buf, "%zu", val.id);
			nd->next = snode_mk(S_INTEGER, buf);
			break;
		}

		case VX_IR_VAL_X86_CC:
			nd->next = snode_mk(S_STRING, val.x86_cc);
			break;

		case VX_IR_VAL_TYPE:
			nd->next = snode_mk(S_STRING, val.ty->debugName);
			break;

		case VX_IR_VAL_BLOCKREF:
			nd->next = snode_mk(S_STRING, val.block->name);
			break;

		case VX_IR_VAL_SYMREF:
			nd->next = snode_mk(S_STRING, val.symref);
			break;

		case VX_IR_VAL_BLOCK:
			nd->next = vx_IrBlock_emitS(cu, val.block);
			break;
	}

	return snode_mk_list(nd);
}

struct SNode* vx_IrNamedValue_emitS(vx_CU* cu, vx_IrNamedValue v)
{
	struct SNode* nd = snode_mk(S_STRING, vx_IrName_str[v.name]);
	nd->next = vx_IrValue_emitS(cu, v.val);
	return snode_mk_list(nd);
}

struct SNode* vx_IrOp_emitS(vx_CU* cu, vx_IrOp* op)
{
	struct SNode* nd = NULL;
	nd = snode_cat(nd, snode_mk_kv("outs", snode_mk_listx(op->outs, op->outs_len, vx_IrTypedVar_emitS, cu)));
	nd = snode_cat(nd, snode_mk_kv("name", snode_mk(S_STRING, vx_IrOpType__entries[op->id].debug.a)));
	nd = snode_cat(nd, snode_mk_kv("params", snode_mk_listx(op->params, op->params_len, vx_IrNamedValue_emitS, cu)));
	nd = snode_cat(nd, snode_mk_kv("args", snode_mk_listx(op->args, op->args_len, vx_IrValue_emitS, cu)));
	return snode_mk_list(nd);
}

struct SNode* vx_IrBlock_emitS(vx_CU* cu, vx_IrBlock* block)
{
	struct SNode* nd = NULL;
	nd = snode_cat(nd, snode_mk_kv("args", snode_mk_listx(block->ins, block->ins_len, vx_IrTypedVar_emitS, cu)));
	nd = snode_cat(nd, snode_mk_kv("ops", snode_mk_listxli(block->first, next, vx_IrOp_emitS, cu)));
	nd = snode_cat(nd, snode_mk_kv("rets", snode_mk_listx(block->outs, block->outs_len, vx_IrVar_emitS, cu)));
	return snode_mk_list(nd);
}
