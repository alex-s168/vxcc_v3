#include <assert.h>

#include "ir.h"


static size_t numDeclaredVarsRec(vx_IrBlock *block)
{
    size_t num = 0;
    for (vx_IrOp* op = block->first; op; op = op->next)
    {
        num += op->outs_len;
        FOR_INPUTS(op, inp, {
            if (inp.type == VX_IR_VAL_BLOCK)
                num += numDeclaredVarsRec(inp.block);
        });
    }
    return num;
}

static void listDeclaredVarsRec(vx_IrBlock* block, vx_IrVar* list, size_t* writer)
{
    for (vx_IrOp* op = block->first; op; op = op->next)
    {
        for (size_t i = 0; i < op->outs_len; i ++)
        {
            list[(*writer)++] = op->outs[i].var;
        }

        FOR_INPUTS(op, inp, {
            if (inp.type == VX_IR_VAL_BLOCK)
                listDeclaredVarsRec(inp.block, list, writer);
        });
    }
}

vx_IrVar* vx_IrBlock_listDeclaredVarsRec(vx_IrBlock *block, size_t * listLenOut)
{
    *listLenOut = numDeclaredVarsRec(block);
    vx_IrVar* list = malloc(sizeof(vx_IrVar) * (*listLenOut));
    size_t writer = 0;
    listDeclaredVarsRec(block, list, &writer);
    assert(writer == *listLenOut);
    return list;
}

vx_IrOp* vx_IrBlock_varDeclNoRec(vx_IrBlock *block, vx_IrVar var)
{
    for (vx_IrOp* op = block->first; op; op = op->next)
        if (vx_IrOp_varInOuts(op, var))
            return op;
    return NULL;
}

vx_IrOp* vx_IrBlock_varDeclRec(vx_IrBlock *block, vx_IrVar var)
{
    for (vx_IrOp* op = block->first; op; op = op->next)
    {
        if (vx_IrOp_varInOuts(op, var))
            return op;

        FOR_INPUTS(op, inp, {
            if (inp.type == VX_IR_VAL_BLOCK) {
                vx_IrOp* decl = vx_IrBlock_varDeclRec(inp.block, var);
                if (decl) return decl;
            }
        });
    }

    return NULL;
}

size_t vx_IrOp_countSuccessors(vx_IrOp *op)
{
    size_t count = 0;
    for (; op; op = op->next) count ++;
    return count;
}

bool VX_IR_OPFILTER_ID__impl(vx_IrOp* op, void* idi) {
    return op->id == (vx_IrOpType) (intptr_t) idi;
}

bool VX_IR_OPFILTER_COMPARISION__impl(vx_IrOp* op, void* ign0) {
    (void) ign0;

    switch (op->id)
    {
        case VX_IR_OP_UGT:
        case VX_IR_OP_UGTE:
        case VX_IR_OP_ULT:
        case VX_IR_OP_ULTE:
        case VX_IR_OP_SGT:
        case VX_IR_OP_SGTE:
        case VX_IR_OP_SLT:
        case VX_IR_OP_SLTE:
        case VX_IR_OP_EQ:
        case VX_IR_OP_NEQ:
            return true;

        default:
            return false;
    }
}

bool VX_IR_OPFILTER_CONDITIONAL__impl(vx_IrOp* op, void* ign1) {
    (void) ign1;

    switch (op->id)
    {
        case VX_IR_OP_COND:
        case VX_IR_OP_CMOV:
            return true;

        default:
            return false;
    }
}

bool VX_IR_OPFILTER_PURE__impl(vx_IrOp* op, void* ign1) {
    (void) ign1;

    return !vx_IrOp_isVolatile(op) && op->id != VX_IR_OP_LOAD && op->id != VX_IR_OP_LOAD_EA;
}

bool VX_IR_OPFILTER_BOTH__impl(vx_IrOp* op, void* arrIn) {
    void **arr = arrIn;

    vx_IrOpFilter fil0 = arr[0];
    vx_IrOpFilter fil1 = arr[2];

    return fil0(op, arr[1]) && fil1(op, arr[3]);
}

bool vx_IrBlock_allMatch(vx_IrOp* first, vx_IrOp* last,
                         vx_IrOpFilter fil, void* data)
{
    for (; first; first = first->next) {
        if (!fil(first, data)) return false;
        if (first == last) break;
    }
    return true;
}

bool vx_IrBlock_noneMatch(vx_IrOp* first, vx_IrOp* last,
                          vx_IrOpFilter fil, void* data)
{
    for (; first; first = first->next) {
        if (fil(first, data)) return false;
        if (first == last) break;
    }
    return true;
}

bool vx_IrBlock_nextOpListBetween(vx_IrBlock* block,
                                  vx_IrOp** first, vx_IrOp** last,
                                  vx_IrOpFilter matchBegin, void *data0,
                                  vx_IrOpFilter matchEnd, void *data1)
{
    vx_IrOp* op = *last ? (*last)->next : block->first;

    for (; op; op = op->next)
    {
        if (matchBegin(op, data0)) break;
    }
    if (op == NULL) return false;
    *first = op;

    for (op = op->next; op; op = op->next)
    {
        if (matchEnd(op, data1)) break;
    }
    if (op == NULL) return false;
    *last = op;

    return true;
}

bool vx_IrOp_inRange(vx_IrOp* op,
                     vx_IrOp* first, vx_IrOp* last)
{
    for (; first; first = first->next) {
        if (first == op) return true;
        if (first == last) break;
    }
    return false;
}


bool vx_IrOp_allDepsInRangeOrArgs(vx_IrBlock* block, vx_IrOp* op,
                                  vx_IrOp* first, vx_IrOp* last)
{
    block = vx_IrBlock_root(block);

    for (size_t i = 0; i < op->args_len; i ++) {
        if (op->args[i].type != VX_IR_VAL_VAR) continue;

        bool found = false;
        for (vx_IrOp* r = first; r; r = r->next)
        {
            for (size_t o = 0; o < r->outs_len; o ++) 
            {
                if (r->outs[o].var == op->args[i].var) {
                    found = true;
                    break;
                }
            }
            if (found) break;
        }

        if (block->as_root.vars[op->args[i].var].decl == NULL)
            found = true; // in block args

        if (!found) return false;
    }
    for (size_t i = 0; i < op->params_len; i ++) {
        if (op->params[i].val.type != VX_IR_VAL_VAR) continue;

        bool found = false;
        for (vx_IrOp* r = first; r; r = r->next)
        {
            for (size_t o = 0; o < r->outs_len; o ++) 
            {
                if (r->outs[o].var == op->params[i].val.var) {
                    found = true;
                    break;
                }
            }
            if (found) break;
        }

        if (block->as_root.vars[op->params[i].val.var].decl == NULL)
            found = true; // in block args

        if (!found) return false;
    }
    return true;
}

static bool anyPlacedIter(vx_IrBlock* block) {
    for (vx_IrOp* op = block->first; op; op = op->next) {
        if (op->id == VX_IR_OP_PLACE)
            return true;

        FOR_INPUTS(op, inp, {
            if (inp.type == VX_IR_VAL_BLOCK)
                if (anyPlacedIter(inp.block))
                    return true;
        });
    }

    return false;
}

bool vx_IrBlock_anyPlaced(vx_IrBlock* block) {
    assert(block->is_root);

    for (vx_IrVar v = 0; v < block->as_root.vars_len; v ++)
        if (block->as_root.vars[v].ever_placed)
            return true;

    return anyPlacedIter(block);
}

vx_IrOp* vx_IrBlock_lastOfType(vx_IrBlock* block, vx_IrOpType type) {
    vx_IrOp* last = NULL;
    for (vx_IrOp* op = block->first; op; op = op->next)
        if (op->id == type)
            last = op;
    return last;
}

bool vx_IrOp_after(vx_IrOp *op, vx_IrOp *after) {
    for (; op; op = op->next)
        if (op == after)
            return true;
    return false;
}

bool vx_IrBlock_vardeclIsInIns(vx_IrBlock *block, vx_IrVar var) {
    for (size_t k = 0; k < block->ins_len; k ++) {
        if (block->ins[k].var == var) {
            return true;
        }
    }
    if (block->parent)
        return vx_IrBlock_vardeclIsInIns(block->parent, var);
    return false;
}

// used for C IR transforms
//
// block is optional
//
// block:
//   __  nested blocks can also exist
//  /\
//    \ search here
//     \
//     before
//
vx_IrOp *vx_IrBlock_vardeclOutBefore(vx_IrBlock *block, vx_IrVar var, vx_IrOp *before) {
    if (before == NULL) {
        before = vx_IrBlock_tail(block);
    }

    for (vx_IrOp* op = vx_IrOp_predecessor(before); op; op = vx_IrOp_predecessor(op)) {
        for (size_t i = 0; i < op->outs_len; i ++)
            if (op->outs[i].var == var)
                return op;

        FOR_INPUTS(op, inp, {
            if (inp.type == VX_IR_VAL_BLOCK) {
                vx_IrOp* d = vx_IrBlock_varDeclRec(inp.block, var);
                if (d) return d;
            }
        });
    }

    if (block->parent == NULL)
        return NULL;

    return vx_IrBlock_vardeclOutBefore(block->parent, var, block->parent_op);
}

/** false for nop and label   true for everything else */
bool vx_IrOpType_hasEffect(vx_IrOpType type)
{
    return vx_IrOpType__entries[type].hasEffect.a;
}

bool vx_IrOp_endsFlow(vx_IrOp *op)
{
    if (op->id == VX_IR_OP_IF)
    {
        vx_IrBlock* pthen = vx_IrOp_param(op, VX_IR_NAME_COND_THEN)->block;
        vx_IrBlock* pelse = vx_IrOp_param(op, VX_IR_NAME_COND_ELSE)->block;
        return vx_IrBlock_endsFlow(pthen) && vx_IrBlock_endsFlow(pelse);
    }
    return vx_IrOpType__entries[op->id].endsFlow.a;
}

bool vx_IrOp_isVolatile(vx_IrOp *op)
{
    if (vx_IrOpType__entries[op->id]._volatile.a)
        return true;

    FOR_INPUTS(op, inp, {
        if (inp.type == VX_IR_VAL_BLOCK)
            if (vx_IrBlock_isVolatile(inp.block))
                return true;
    });

    return false;
}

bool vx_IrOp_hasSideEffect(vx_IrOp *op)
{
    if (vx_IrOpType__entries[op->id].sideEffect.a)
        return true;

    FOR_INPUTS(op, inp, {
        if (inp.type == VX_IR_VAL_BLOCK)
            if (vx_IrBlock_hasSideEffect(inp.block))
                return true;
    });

    return false;
}

size_t vx_IrOp_inlineCost(vx_IrOp *op)
{
    size_t sum = vx_IrOpType__entries[op->id].inlineCost.a;
    FOR_INPUTS(op, inp, {
        if (inp.type == VX_IR_VAL_BLOCK)
            sum += vx_IrBlock_inlineCost(inp.block);
    });
    return sum;
}

size_t vx_IrOp_execCost(vx_IrOp *op)
{
    size_t sum = vx_IrOpType__entries[op->id].execCost.a;
    FOR_INPUTS(op, inp, {
        if (inp.type == VX_IR_VAL_BLOCK)
            sum += vx_IrBlock_execCost(inp.block);
    });
    return sum;
}


bool vx_IrBlock_endsFlow(vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next)
        if (vx_IrOp_endsFlow(op))
            return true;
    return false;
}

bool vx_IrBlock_isVolatile(vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next)
        if (vx_IrOp_isVolatile(op))
            return true;
    return false;
}

bool vx_IrBlock_hasSideEffect(vx_IrBlock *block)
{
    for (vx_IrOp *op = block->first; op; op = op->next)
        if (vx_IrOp_hasSideEffect(op))
            return true;
    return false;
}

size_t vx_IrBlock_inlineCost(vx_IrBlock *block)
{
    size_t total = 0;
    for (vx_IrOp *op = block->first; op; op = op->next)
    {
        total += vx_IrOp_inlineCost(op);
    }
    return total;
}

size_t vx_IrBlock_execCost(vx_IrBlock *block)
{
    size_t total = 0;
    for (vx_IrOp *op = block->first; op; op = op->next)
    {
        total += vx_IrOp_execCost(op);
    }
    return total;
}

bool vx_IrOp_varInOuts(const vx_IrOp *op, vx_IrVar var) {
    for (size_t i = 0; i < op->outs_len; i ++)
        if (op->outs[i].var == var)
            return true;
    return false;
}

static bool var_used_val(vx_IrValue val, vx_IrVar var) {
    if (val.type == VX_IR_VAL_BLOCK)
        return vx_IrBlock_varUsed(val.block, var);
    if (val.type == VX_IR_VAL_VAR)
        return val.var == var;
    return false;
}

bool vx_IrOp_varUsed(const vx_IrOp *op, vx_IrVar var) {
    FOR_INPUTS(op, inp, ({
        if (var_used_val(inp, var))
            return true;
    }));
    return false;
}

bool vx_IrBlock_varUsed(vx_IrBlock *block, vx_IrVar var)
{
    assert(block);

    for (size_t i = 0; i < block->outs_len; i++)
        if (block->outs[i] == var)
            return true;

    for (vx_IrOp *op = block->first; op; op = op->next) {
        if (vx_IrOp_varUsed(op, var))
            return true;
    }

    return false;
}

static vx_IrType* typeofvar(vx_IrBlock* block, vx_IrVar var) {
    if (!block) {
        return NULL;
    }

    for (size_t i = 0; i < block->ins_len; i ++) {
        if (block->ins[i].var == var) {
            return block->ins[i].type;
        }
    }

    for (vx_IrOp* op = block->first; op; op = op->next) {
        for (size_t i = 0; i < op->outs_len; i ++)
            if (op->outs[i].var == var)
                return op->outs[i].type;
    
        for (size_t i = 0; i < op->params_len; i ++) {
            if (op->params[i].val.type == VX_IR_VAL_BLOCK) {
                vx_IrType* ty = typeofvar(op->params[i].val.block, var);
                if (ty) return ty;
            }
        }

        for (size_t i = 0; i < op->args_len; i ++) {
            if (op->args[i].type == VX_IR_VAL_BLOCK) {
                vx_IrType* ty = typeofvar(op->args[i].block, var);
                if (ty) return ty;
            }
        }
    }

    return NULL;
}

vx_IrType *vx_IrBlock_typeofVar(vx_IrBlock *block, vx_IrVar var) {
    vx_IrBlock* root = vx_IrBlock_root(block);
    if (root && root->is_root && root->as_root.vars[var].ll_type) {
        return block->as_root.vars[var].ll_type;
    }

    return typeofvar(root, var);
}

vx_IrOp* vx_IrOp_nextWithEffect(vx_IrOp* op) {
    for (op = op->next; op; op = op->next)
        if (vx_IrOpType_hasEffect(op->id))
            return op;
    return NULL;
}

bool vx_IrOp_followingNoEffect(vx_IrOp* op) {
    return vx_IrOp_nextWithEffect(op) == NULL;
}

static bool is_tail__rec(vx_IrBlock *block, vx_IrOp *op) {
    if (!op)
        return true;
    if (vx_IrOp_followingNoEffect(op)) {
        if (block->parent)
            return is_tail__rec(block->parent, block->parent_op);
        return true;
    }
    return false;
}

bool vx_IrOp_isTail(vx_IrOp *op) {
    if (is_tail__rec(op->parent, op)) {
        return true;
    }

    vx_IrOp* effect = vx_IrOp_nextWithEffect(op);
    if (effect == NULL)
        return false;

    vx_IrBlock* root = vx_IrBlock_root(op->parent);
    assert(root);

    if (effect->id == VX_IR_OP_GOTO) {
        size_t label = vx_IrOp_param(effect, VX_IR_NAME_ID)->id;
        vx_IrOp* dest = root->as_root.labels[label].decl;
        return vx_IrOp_isTail(dest);
    }

    return false;
}

bool vx_IrBlock_llIsLeaf(vx_IrBlock* block) {
    for (vx_IrOp* op = block->first; op; op = op->next) {
        switch (op->id) {
            case VX_IR_OP_CALL:
            case VX_IR_OP_TAILCALL:
                return false;

            default:
                break;
        }
    }

    return true;
}

static void recursive_heat(vx_IrBlock* root, vx_IrBlock* block, size_t factor)
{
	for (vx_IrOp* op = block->first; op; op = op->next)
	{
		FOR_INPUTS(op, input, ({
			if (input.type == VX_IR_VAL_VAR) {
				root->as_root.vars[input.var].heat += factor;
			} else if (input.type == VX_IR_VAL_BLOCK) {
				double innerFactor;

				// TODO: move to cdef after defaults supported
				switch (op->id) {
				case VX_IR_OP_IF:
				case VX_IR_OP_CMOV:
					innerFactor = 0.5;
					break;

				case VX_IR_OP_WHILE:
				case VX_IR_OP_CFOR:
				case VX_IR_OP_REPEAT:
					innerFactor = 20;
					break;

				default:
					innerFactor = 1; 
					break;
				}

				recursive_heat(root, input.block, (size_t) (factor * innerFactor));
			}
		}));

		for (size_t i = 0; i < op->outs_len; i ++)
			root->as_root.vars[op->outs[i].var].heat += factor;
	}
}

void vx_IrBlock_root_varsHeat(vx_IrBlock* block)
{
	assert(block->is_root);

	for (size_t i = 0; i < block->as_root.vars_len; i ++)
		block->as_root.vars[i].heat = 0;

	recursive_heat(block, block, 1);
}
