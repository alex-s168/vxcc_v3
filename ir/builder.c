#include "ir.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

void vx_IrBlock_init(vx_IrBlock *block,
                     vx_IrBlock *parent,
                      vx_IrOp *parent_op)
{
    block->parent = parent;
    block->parent_op = parent_op;

    block->is_root = (parent == NULL);
    memset(&block->as_root, 0, sizeof(block->as_root));

    block->ins = NULL;
    block->ins_len = 0;

    block->first = NULL;

    block->outs = NULL;
    block->outs_len = 0;

    block->should_free = false;

    block->name = "___anon";
}

vx_IrBlock *vx_IrBlock_initHeap(vx_IrBlock *parent, vx_IrOp *parent_op)
{
    vx_IrBlock *new = malloc(sizeof(vx_IrBlock));
    if (new == NULL)
        return NULL;
    vx_IrBlock_init(new, parent, parent_op);
    new->should_free = true;
    return new;
}

static vx_IrOp *find_varDecl(const vx_IrBlock *block, const vx_IrVar var)
{
    for (vx_IrOp *op = block->first; op; op = op->next) {
        for (size_t j = 0; j < op->outs_len; j ++)
            if (op->outs[j].var == var)
                return op;

        for (size_t j = 0; j < op->params_len; j ++) {
            const vx_IrValue param = op->params[j].val;

            if (param.type == VX_IR_VAL_BLOCK) {
                for (size_t k = 0; k < param.block->ins_len; k ++)
                    if (param.block->ins[k].var == var)
                        return op;

                vx_IrOp *res = find_varDecl(param.block, var);
                if (res != NULL)
                    return res;
            }
        }
    }

    return NULL;
}

void vx_IrBlock_makeRoot(vx_IrBlock *block,
                         const size_t total_vars)
{
    assert(block->parent == NULL);

    block->is_root = true;

    block->as_root.vars_len = total_vars;
    block->as_root.vars = calloc(total_vars, sizeof(*block->as_root.vars));
    for (size_t i = 0; i < total_vars; i ++) {
        vx_IrOp *decl = find_varDecl(block, i);
        if (decl == NULL) {
            block->as_root.vars[i].decl = NULL;
            continue;
        }
        block->as_root.vars[i].decl = decl;
    }

    block->as_root.labels = NULL;
    block->as_root.labels_len = 0;
}

void vx_IrBlock_addIn(vx_IrBlock *block,
                       const vx_IrVar var,
                       vx_IrType *ty)
{
    block->ins = realloc(block->ins, sizeof(vx_IrTypedVar) * (block->ins_len + 1));
    vx_IrTypedVar* v = &block->ins[block->ins_len ++];
    v->var = var;
    v->type = ty;
}

void vx_IrBlock_putVar(vx_IrBlock *root, vx_IrVar var, vx_IrOp *decl) {
    assert(root->is_root);
    if (var >= root->as_root.vars_len) {
        root->as_root.vars = realloc(root->as_root.vars, sizeof(*root->as_root.vars) * (var + 1));
	root->as_root.vars_len = var + 1;
    }
    memset(&root->as_root.vars[var], 0, sizeof(*root->as_root.vars));
    root->as_root.vars[var].decl = decl;
}

void vx_IrBlock_putLabel(vx_IrBlock *root, size_t label, vx_IrOp *decl) {
    assert(root->is_root);
    if (label >= root->as_root.labels_len) {
        root->as_root.labels = realloc(root->as_root.labels, sizeof(*root->as_root.labels) * (label + 1));
        root->as_root.labels_len = label + 1;
    }
    root->as_root.labels[label].decl = decl;
}

struct add_op__data {
    vx_IrBlock *root; 
    vx_IrOp *new;
};

static bool add_op__trav(vx_IrOp *op, void *dataIn) {
    struct add_op__data *data = dataIn;

    for (size_t i = 0; i < op->outs_len; i ++) {
        vx_IrTypedVar vv = op->outs[i];
        vx_IrBlock_putVar(data->root, vv.var, op);
    }

    if (op->id == VX_LIR_OP_LABEL) {
        size_t label = vx_IrOp_param(op, VX_IR_NAME_ID)->id;
        vx_IrBlock_putLabel(data->root, label, op);
    }

    return false;
}

void vx_IrBlock_addOp(vx_IrBlock *block,
                       const vx_IrOp *op)
{
    vx_IrOp *new = vx_IrBlock_addOpBuilding(block);
    *new = *op;

    new->next = NULL;
    new->parent = block;

    // make sure that out variables and labels are in root block (add them if not)

    struct add_op__data data;
    data.new = new;
    data.root = (vx_IrBlock*) vx_IrBlock_root(block->parent);

    if (data.root == NULL)
        return;

    vx_IrBlock_deepTraverse(new->parent, add_op__trav, &data);
}

/** WARNING: DON'T REF VARS IN OP THAT ARE NOT ALREADY INDEXED ROOT */
vx_IrOp *vx_IrBlock_addOpBuilding(vx_IrBlock *block) {
    vx_IrOp *new = fastalloc(sizeof(vx_IrOp));
    new->next = NULL;

    vx_IrOp *end = vx_IrBlock_tail(block);

    if (end == NULL) {
        block->first = new;
    } else {
        end->next = new;
    }

    return new;
}

vx_IrOp *vx_IrBlock_insertOpBuildingAfter(vx_IrOp *after) {
    assert(after);
    vx_IrBlock *block = after->parent;
    assert(block);

    vx_IrOp *new = fastalloc(sizeof(vx_IrOp));

    new->next = after->next;

    after->next = new;

    return new;
}

void vx_IrBlock_addOut(vx_IrBlock *block, vx_IrVar out)
{
    block->outs = realloc(block->outs, sizeof(vx_IrVar) * (block->outs_len + 1));
    block->outs[block->outs_len ++] = out;
}

void vx_IrBlock_destroy(vx_IrBlock *block)
{
    if (block == NULL)
        return;
    free(block->ins);
    for (vx_IrOp *op = block->first; op; op = op->next)
        vx_IrOp_destroy(op);
    free(block->outs);
    if (block->is_root) {
        free(block->as_root.vars);
	free(block->as_root.labels);
    }
    if (block->should_free)
        free(block);
}

vx_IrValue *vx_IrOp_param(const vx_IrOp *op,
                          vx_IrName name)
{
    for (size_t i = 0; i < op->params_len; i ++)
        if (op->params[i].name == name)
            return &op->params[i].val;

    return NULL;
}

void vx_IrNamedValue_destroy(vx_IrNamedValue value)
{
    if (value.val.type == VX_IR_VAL_BLOCK)
        vx_IrBlock_destroy(value.val.block);
}

void vx_IrOp_init(vx_IrOp *op,
                  const vx_IrOpType type,
                  vx_IrBlock *parent)
{
    op->next = NULL;

    op->outs = NULL;
    op->outs_len = 0;

    op->params = NULL;
    op->params_len = 0;

    op->id = type;

    op->parent = parent;

    op->args = NULL;
    op->args_len = 0;
}

void vx_IrOp_addOut(vx_IrOp *op,
                     vx_IrVar var,
                     vx_IrType *type)
{
    op->outs = realloc(op->outs, sizeof(vx_IrTypedVar) * (op->outs_len + 1));
    op->outs[op->outs_len ++] = (vx_IrTypedVar) { .var = var, .type = type };
}

void vx_IrOp_addParam(vx_IrOp *op,
                       vx_IrNamedValue p)
{
    op->params = realloc(op->params, sizeof(vx_IrNamedValue) * (op->params_len + 1));
    op->params[op->params_len ++] = p;
}

void vx_IrOp_addParam_s(vx_IrOp *op,
                         vx_IrName name,
                         const vx_IrValue val)
{
    vx_IrOp_addParam(op, vx_IrNamedValue_create(name, val));
}

void vx_IrOp_addArg(vx_IrOp *op, vx_IrValue val)
{
    op->args = realloc(op->args, sizeof(vx_IrValue) * (op->args_len + 1));
    op->args[op->args_len ++] = val;
}

void vx_IrOp_undeclare(vx_IrOp *op)
{
    vx_IrBlock *root = (vx_IrBlock*) vx_IrBlock_root(op->parent);
    if (root) {
        for (size_t i = 0; i < op->outs_len; i ++) { 
            vx_IrVar var = op->outs[i].var;
            if (var < root->as_root.vars_len && root->as_root.vars[var].decl == op) {
                root->as_root.vars[var].decl = NULL;
            }
        }

        if (op->id == VX_LIR_OP_LABEL) {
            size_t label = vx_IrOp_param(op, VX_IR_NAME_ID)->id;
            if (label < root->as_root.labels_len && root->as_root.labels[label].decl == op) {
                root->as_root.labels[label].decl = NULL;
            }
        }
    }
}

void vx_IrOp_destroy(vx_IrOp *op)
{
    // BEFORE CHANGING NOTE THAT MOST PASSES MISUSE THIS FUNCTION!!!
    vx_IrOp_removeParams(op);
    free(op->outs);
    free(op->args);
}

void vx_IrOp_removeParams(vx_IrOp *op)
{
    for (size_t i = 0; i < op->params_len; i ++)
        vx_IrNamedValue_destroy(op->params[i]);
    free(op->params);
    op->params = NULL;
    op->params_len = 0;
}

void vx_IrOp_stealOuts(vx_IrOp *dest, const vx_IrOp *src)
{
    for (size_t i = 0; i < src->outs_len; i ++) {
        vx_IrOp_addOut(dest, src->outs[i].var, src->outs[i].type);
    }
}

void vx_IrOp_removeOutAt(vx_IrOp *op, const size_t id)
{
    if (op && op->outs) {
        memmove(op->outs + id, op->outs + id + 1, sizeof(vx_IrTypedVar) * (op->outs_len - id - 1));
        op->outs_len --;
    }
}

void vx_IrBlock_removeOutAt(vx_IrBlock *block, size_t id)
{
    if (block && block->outs) {
        memmove(block->outs + id, block->outs + id + 1, sizeof(vx_IrVar) * (block->outs_len - id - 1));
        block->outs_len --;
    }
}

void vx_IrOp_removeParamAt(vx_IrOp *op, const size_t id)
{
    if (op && op->params) {
        memmove(op->params + id, op->params + id + 1, sizeof(vx_IrNamedValue) * (op->params_len - id - 1));
        op->params_len --;
    }
}

void vx_IrOp_removeParam(vx_IrOp *op, vx_IrName param) {
    for (size_t i = 0; i < op->params_len; i ++) {
        if (op->params[i].name == param) {
            vx_IrOp_removeParamAt(op, i);
            break;
        }
    }
}


void vx_IrOp_removeArgAt(vx_IrOp *op, const size_t id)
{
    if (op && op->args) {
        memmove(op->args + id, op->args + id + 1, sizeof(vx_IrNamedValue) * (op->args_len - id - 1));
        op->args_len --;
    }
}

void vx_IrOp_stealArgs(vx_IrOp *dest, const vx_IrOp *src)
{
    free(dest->args);
    dest->args = malloc(sizeof(vx_IrValue) * src->args_len);
    assert(dest->args);
    for (size_t i = 0; i < src->args_len; i ++)
        dest->args[i] = src->args[i];
    dest->args_len = src->args_len;
}

void vx_IrOp_removeSuccessor(vx_IrOp *op) {
    if (op && op->next) {
        vx_IrOp *newnext = op->next->next;
        vx_IrOp *old = op->next;
        (void) old;

        // WE DON'T DO THAT BECAUSE WE CALL REMOVE WHILE ITERATING IN THE PASSES
        // old->next = NULL;
        
        op->next = newnext;
    }
}

vx_IrOp *vx_IrOp_predecessor(vx_IrOp *op) {
    if (op) {
        assert(op->parent);
        if (op->parent->first == op)
            return NULL;
        for (vx_IrOp *curr = op->parent->first; curr; curr = curr->next)
            if (curr->next == op)
                return curr;
    }
    return NULL;
}

/** should use remove_successor whenever possible! */
void vx_IrOp_remove(vx_IrOp *op) {
    if (op == NULL || op->parent == NULL)
        return;

    if (op->parent->first == op) {
        op->parent->first = op->next;
        return;
    }

    vx_IrOp *pred = vx_IrOp_predecessor(op);
    if (pred == NULL)
        return;

    vx_IrOp_removeSuccessor(pred);
}
