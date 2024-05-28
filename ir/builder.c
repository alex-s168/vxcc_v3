#include "ir.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

void vx_IrBlock_init(vx_IrBlock *block,
                     vx_IrBlock *parent,
                     size_t parent_index)
{
    block->parent = parent;
    block->parent_index = parent_index;

    block->is_root = false;

    block->ins = NULL;
    block->ins_len = 0;

    block->ops = NULL;
    block->ops_len = 0;

    block->outs = NULL;
    block->outs_len = 0;

    block->should_free = false;
}

vx_IrBlock *vx_IrBlock_init_heap(vx_IrBlock *parent,
                                 size_t parent_index)
{
    vx_IrBlock *new = malloc(sizeof(vx_IrBlock));
    if (new == NULL)
        return NULL;
    vx_IrBlock_init(new, parent, parent_index);
    new->should_free = true;
    return new;
}

void vx_IrBlock_make_root(vx_IrBlock *block,
                          const size_t total_vars)
{
    assert(block->parent == NULL);

    block->is_root = true;

    block->as_root.vars_len = total_vars;
    block->as_root.vars = malloc(sizeof(*block->as_root.vars) * total_vars);
    for (size_t i = 0; i < total_vars; i ++) {
        vx_IrOp *decl = vx_IrBlock_find_var_decl(block, i);
        if (decl == NULL) {
            block->as_root.vars[i].decl_parent = NULL;
            continue;
        }
        vx_IrBlock_root_set_var_decl(block, i, decl);
    }

    block->as_root.labels = NULL;
    block->as_root.labels_len = 0;
}

void vx_IrBlock_add_in(vx_IrBlock *block,
                       const vx_IrVar var)
{
    block->ins = realloc(block->ins, sizeof(vx_IrVar) * (block->ins_len + 1));
    block->ins[block->ins_len ++] = var;
}

static void root_block_put_var(vx_IrBlock *root, vx_IrVar var, vx_IrOp *decl) {
    assert(root->is_root);
    if (var >= root->as_root.vars_len) {
        root->as_root.vars = realloc(root->as_root.vars, sizeof(*root->as_root.vars) * (var + 1));
        root->as_root.vars_len = var + 1;
    }
    vx_IrBlock_root_set_var_decl(root, var, decl);
}

struct add_op__data {
    vx_IrBlock *root; 
    vx_IrOp *new;
};

static void add_op__trav(vx_IrOp *op, void *dataIn) {
    struct add_op__data *data = dataIn;

    for (size_t i = 0; i < op->outs_len; i ++) {
        vx_IrTypedVar vv = op->outs[i];
        root_block_put_var(data->root, vv.var, op);
    }

    // TODO: also fix labels
}

void vx_IrBlock_add_op(vx_IrBlock *block,
                       const vx_IrOp *op)
{
    vx_IrOp *new = vx_IrBlock_add_op_building(block);
    *new = *op;

    new->parent = block;

    // make sure that out variables and labels are in root block (add them if not)

    struct add_op__data data;
    data.new = new;
    data.root = (vx_IrBlock*) vx_IrBlock_root(block->parent);

    if (data.root == NULL)
        return;

    vx_IrView_deep_traverse(vx_IrView_of_single(new->parent, new - block->ops), add_op__trav, &data);
}

/** WARNING: DON'T REF VARS IN OP THAT ARE NOT ALREADY INDEXED ROOT */
vx_IrOp *vx_IrBlock_add_op_building(vx_IrBlock *block) {
    block->ops = realloc(block->ops, sizeof(vx_IrOp) * (block->ops_len + 1));
    return &block->ops[block->ops_len ++];
}

void vx_IrBlock_add_all_op(vx_IrBlock *dest,
                           const vx_IrBlock *src)
{
    dest->ops = realloc(dest->ops, sizeof(vx_IrOp) * (dest->ops_len + src->ops_len));
    memcpy(dest->ops + dest->ops_len, src->ops, src->ops_len);
    for (size_t i = dest->ops_len; i < dest->ops_len + src->ops_len; i ++) {
        dest->ops[i].parent = dest;
    }
    dest->ops_len += src->ops_len;
}

void vx_IrBlock_add_out(vx_IrBlock *block,
                        vx_IrVar out)
{
    block->outs = realloc(block->outs, sizeof(vx_IrVar) * (block->outs_len + 1));
    block->outs[block->outs_len ++] = out;
}

void vx_IrBlock_destroy(vx_IrBlock *block)
{
    if (block == NULL)
        return;
    free(block->ins);
    for (size_t i = 0; i < block->ops_len; i ++)
        vx_IrOp_destroy(&block->ops[i]);
    free(block->ops);
    free(block->outs);
    if (block->is_root)
        free(block->as_root.vars);
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
    op->outs = NULL;
    op->outs_len = 0;

    op->params = NULL;
    op->params_len = 0;

    op->id = type;

    op->parent = parent;

    op->states = NULL;
    op->states_len = 0;

    op->info = vx_OpInfoList_create();
}

void vx_IrOp_add_out(vx_IrOp *op,
                     vx_IrVar var,
                     vx_IrType *type)
{
    op->outs = realloc(op->outs, sizeof(vx_IrTypedVar) * (op->outs_len + 1));
    op->outs[op->outs_len ++] = (vx_IrTypedVar) { .var = var, .type = type };
}

void vx_IrOp_add_param(vx_IrOp *op,
                       vx_IrNamedValue p)
{
    op->params = realloc(op->params, sizeof(vx_IrNamedValue) * (op->params_len + 1));
    op->params[op->params_len ++] = p;
}

void vx_IrOp_add_param_s(vx_IrOp *op,
                         vx_IrName name,
                         const vx_IrValue val)
{
    vx_IrOp_add_param(op, vx_IrNamedValue_create(name, val));
}

void vx_IrOp_undeclare(vx_IrOp *op)
{
    vx_IrBlock *root = (vx_IrBlock*) vx_IrBlock_root(op->parent);
    if (root) {
        for (size_t i = 0; i < op->outs_len; i ++) { 
            vx_IrVar var = op->outs[i].var;
            if (var < root->as_root.vars_len && vx_IrBlock_root_get_var_decl(root, var) == op) {
                root->as_root.vars[var].decl_parent = NULL;
            }
        }
    }
}

void vx_IrOp_destroy(vx_IrOp *op)
{
    // BEFORE CHANGING NOTE THAT MOST PASSES MISUSE THIS FUNCTION!!!
    vx_IrOp_remove_params(op);
    free(op->outs);
    free(op->states);
    vx_OpInfoList_destroy(&op->info);
}

void vx_IrOp_remove_params(vx_IrOp *op)
{
    for (size_t i = 0; i < op->params_len; i ++)
        vx_IrNamedValue_destroy(op->params[i]);
    free(op->params);
    op->params = NULL;
    op->params_len = 0;
}


void vx_IrOp_steal_outs(vx_IrOp *dest, const vx_IrOp *src)
{
    for (size_t i = 0; i < src->outs_len; i ++) {
        vx_IrOp_add_out(dest, src->outs[i].var, src->outs[i].type);
    }
}

void vx_IrOp_remove_out_at(vx_IrOp *op,
                           const size_t id)
{
    memmove(op->outs + id, op->outs + id + 1, sizeof(vx_IrTypedVar) * (op->outs_len - id - 1));
    op->outs_len --;
}

void vx_IrBlock_remove_out_at(vx_IrBlock *block,
                              size_t id)
{
    memmove(block->outs + id, block->outs + id + 1, sizeof(vx_IrVar) * (block->outs_len - id - 1));
    block->outs_len --;
}

void vx_IrOp_remove_param_at(vx_IrOp *op,
                             const size_t id)
{
    memmove(op->params + id, op->params + id + 1, sizeof(vx_IrNamedValue) * (op->params_len - id - 1));
    op->params_len --;
}


void vx_IrOp_remove_state_at(vx_IrOp *op,
                             const size_t id)
{
    memmove(op->states + id, op->states + id + 1, sizeof(vx_IrNamedValue) * (op->states_len - id - 1));
    op->states_len --;
}

void vx_IrOp_steal_states(vx_IrOp *dest,
                          const vx_IrOp *src)
{
    free(dest->states);
    dest->states = malloc(sizeof(vx_IrValue) * src->states_len);
    for (size_t i = 0; i < src->states_len; i ++)
        dest->states[i] = src->states[i];
    dest->states_len = src->states_len;
}
