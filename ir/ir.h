#ifndef IR_H
#define IR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TOOD: conditional tailcall & use in ssa->ll lowering


// not all things should use these functions 
// main usages: codegen

/** alloc things that are not meant to be freed before end of compilation */
void * fastalloc(size_t bytes);

void fastfreeall(void);



#include "../common.h"
#include "../cg/cg.h"

struct vx_IrOp_s;
typedef struct vx_IrOp_s vx_IrOp;

typedef size_t vx_IrVar;

typedef struct {
    vx_IrVar var;
    bool present;
} vx_OptIrVar;

#define VX_IRVAR_OPT_NONE (vx_OptIrVar) {.present = false,.var = 0}
#define VX_IRVAR_OPT_SOME(v) (vx_OptIrVar) {.present = true,.var = v}

typedef struct vx_IrType_s vx_IrType;

// only in C IR
typedef struct {
    vx_IrType **members;
    size_t      members_len;

    bool pack;
    size_t align;
} vx_IrTypeCIRStruct;

// only in C IR
typedef struct {
    vx_IrType **members;
    size_t      members_len;

    size_t algin;
} vx_IrTypeCIRUnion;

typedef struct {
    bool   sizeless;
    size_t size;
    size_t align;
    size_t pad;
} vx_IrTypeBase;

struct vx_IrType_s {
    const char *debugName;

    enum {
        // present in: cir
        VX_IR_TYPE_KIND_CIR_STRUCT,
        VX_IR_TYPE_KIND_CIR_UNION,

        // present in: cir, ssa
        VX_IR_TYPE_KIND_BASE,
    } kind;

    union {
        vx_IrTypeBase       base;
        vx_IrTypeCIRStruct  cir_struct;
        vx_IrTypeCIRUnion   cir_union;
    };
};

static vx_IrType* vx_IrType_heap(void) {
    return (vx_IrType*) memset(malloc(sizeof(vx_IrType)), 0, sizeof(vx_IrType));
}

// TODO: remove cir checks and make sure fn called after cir type expand & MAKE TYPE EXPAND AWARE OF MEMBER ALIGN FOR UNIONS
static size_t vx_IrType_size(vx_IrType *ty) {
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
    }
}

static bool vx_IrType_compatible(vx_IrType *a, vx_IrType *b) {
    return a == b; // TODO (not used right now)
}

typedef struct {
    vx_IrOp *first;
    vx_IrOp *last;
} lifetime;

struct vx_IrBlock_s;
typedef struct vx_IrBlock_s vx_IrBlock;

struct vx_IrBlock_s {
    vx_IrBlock *parent;
    size_t parent_index;

    bool is_root;
    struct {
        struct {
            vx_IrBlock *decl_parent;
            size_t      decl_idx;

            lifetime  ll_lifetime;
            vx_IrType *ll_type;
        } *vars;
        size_t vars_len;

        struct {
            vx_IrBlock *decl_parent;
            size_t      decl_idx;
        } *labels;
        size_t labels_len;
    } as_root;
    
    vx_IrVar *ins;
    size_t    ins_len;
    
    vx_IrOp *ops;
    size_t   ops_len;
    
    vx_IrVar *outs;
    size_t    outs_len;

    bool should_free;
};

const vx_IrBlock *vx_IrBlock_root(const vx_IrBlock *block);

// TODO: do differently

vx_Errors vx_IrBlock_verify(const vx_IrBlock *block, vx_OpPath path);

static int vx_ir_verify(const vx_IrBlock *block) {
    vx_OpPath path;
    path.ids = NULL;
    path.len = 0;
    const vx_Errors errs = vx_IrBlock_verify(block, path);
    vx_Errors_print(errs, stderr);
    vx_Errors_free(errs);
    return errs.len > 0;
}

// TODO: move builder functions into separate header

/** DON'T RUN INIT AFTERWARDS */
vx_IrBlock *vx_IrBlock_init_heap(vx_IrBlock *parent, size_t parent_index);
void vx_IrBlock_init(vx_IrBlock *block, vx_IrBlock *parent, size_t parent_index);
/** run AFTER you finished building it! */
void vx_IrBlock_make_root(vx_IrBlock *block, size_t total_vars);
void vx_IrBlock_add_in(vx_IrBlock *block, vx_IrVar var);
void vx_IrBlock_add_op(vx_IrBlock *block, const vx_IrOp *op);
/** WARNING: DON'T REF VARS IN OP THAT ARE NOT ALREADY INDEXED ROOT */
vx_IrOp *vx_IrBlock_add_op_building(vx_IrBlock *block);
void vx_IrBlock_add_all_op(vx_IrBlock *dest, const vx_IrBlock *src);
void vx_IrBlock_add_out(vx_IrBlock *block, vx_IrVar out);
void vx_IrBlock_destroy(vx_IrBlock *block);
vx_IrType *vx_IrBlock_typeof_var(vx_IrBlock *block, vx_IrVar var);

vx_IrVar vx_IrBlock_new_var(vx_IrBlock *block, vx_IrOp *decl);
size_t   vx_IrBlock_new_label(vx_IrBlock *block, vx_IrOp *decl);
void vx_IrBlock_swap_in_at(vx_IrBlock *block, size_t a, size_t b);
void vx_IrBlock_swap_out_at(vx_IrBlock *block, size_t a, size_t b);
void vx_IrBlock_remove_out_at(vx_IrBlock *block, size_t id);
size_t vx_IrBlock_insert_label_op(vx_IrBlock *block);

bool vx_IrBlock_var_used(const vx_IrBlock *block, vx_IrVar var);
bool vx_IrOp_var_used(const vx_IrOp *op, vx_IrVar var);

void vx_IrBlock_dump(const vx_IrBlock *block, FILE *out, size_t indent);

typedef struct {
    enum {
        // storable
        VX_IR_VAL_IMM_INT,
        VX_IR_VAL_IMM_FLT,
        VX_IR_VAL_VAR,
        VX_IR_VAL_UNINIT,

        // not storable
        VX_IR_VAL_BLOCK,
        VX_IR_VAL_TYPE,
        VX_IR_VAL_ID,
    } type;

    union {
        long long imm_int;
        double imm_flt;
        vx_IrVar var;

        vx_IrBlock *block;
        vx_IrType *ty;
        size_t id;
    };
} vx_IrValue;

vx_IrValue vx_IrValue_clone(vx_IrValue value);
void vx_IrValue_dump(vx_IrValue value, FILE *out, size_t indent);

vx_IrOp *vx_IrBlock_find_var_decl(const vx_IrBlock *block, vx_IrVar var);
/** returns true if static eval ok; only touches dest if true */
bool vx_IrBlock_eval_var(const vx_IrBlock *block, vx_IrVar var, vx_IrValue *dest);
bool vx_Irblock_mightbe_var(const vx_IrBlock *block, vx_IrVar var, vx_IrValue v);
bool vx_Irblock_alwaysis_var(const vx_IrBlock *block, vx_IrVar var, vx_IrValue v);
void vx_Irblock_eval(vx_IrBlock *block, vx_IrValue *v);

typedef enum {
    VX_IR_NAME_OPERAND_A,
    VX_IR_NAME_OPERAND_B,

    VX_IR_NAME_BLOCK,
    VX_IR_NAME_VALUE,
    VX_IR_NAME_ADDR,
    VX_IR_NAME_COND,
    VX_IR_NAME_VAR,

    VX_IR_NAME_COND_THEN,
    VX_IR_NAME_COND_ELSE,

    VX_IR_NAME_LOOP_DO,
    VX_IR_NAME_LOOP_START,
    VX_IR_NAME_LOOP_ENDEX,
    VX_IR_NAME_LOOP_STRIDE,

    VX_IR_NAME_ALTERNATIVE_A,
    VX_IR_NAME_ALTERNATIVE_B,

    VX_IR_NAME_ID,
} vx_IrName;

extern const char *vx_IrName_str[];

typedef struct {
    vx_IrName   name;
    vx_IrValue  val;
} vx_IrNamedValue;

static vx_IrNamedValue vx_IrNamedValue_create(vx_IrName name, vx_IrValue v) {
    return (vx_IrNamedValue) {
        .name = name,
        .val = v,
    };
}
void vx_IrNamedValue_destroy(vx_IrNamedValue v);

typedef enum {
    VX_IR_OP_NOP = 0,
    VX_IR_OP_IMM,            // "val"
    VX_IR_OP_FLATTEN_PLEASE, // "block"
    
    // convert
    /** for pointers only! */
    VX_IR_OP_REINTERPRET, // "val"
    VX_IR_OP_ZEROEXT,     // "val"
    VX_IR_OP_SIGNEXT,     // "val"
    VX_IR_OP_TOFLT,       // "val"
    VX_IR_OP_FROMFLT,     // "val"
    VX_IR_OP_BITCAST,     // "val"

    // mem
    VX_IR_OP_LOAD,            // "addr"
    VX_IR_OP_LOAD_VOLATILE,   // "addr"
    VX_IR_OP_STORE,           // "addr", "val"
    VX_IR_OP_STORE_VOLATILE,  // "addr", "val"
    VX_IR_OP_PLACE,           // "var"
    
    // arithm
    VX_IR_OP_ADD, // "a", "b"
    VX_IR_OP_SUB, // "a", "b"
    VX_IR_OP_MUL, // "a", "b"
    VX_IR_OP_DIV, // "a", "b"
    VX_IR_OP_MOD, // "a", "b"

    // compare
    VX_IR_OP_GT,  // "a", "b"
    VX_IR_OP_GTE, // "a", "b"
    VX_IR_OP_LT,  // "a", "b"
    VX_IR_OP_LTE, // "a", "b"
    VX_IR_OP_EQ,  // "a", "b"
    VX_IR_OP_NEQ, // "a", "b"

    // boolean
    VX_IR_OP_NOT, // "val"
    VX_IR_OP_AND, // "val"
    VX_IR_OP_OR,  // "val"

    // bitwise boolean
    VX_IR_OP_BITWISE_NOT, // "val"
    VX_IR_OP_BITWISE_AND, // "a", "b"
    VX_IR_OP_BITIWSE_OR,  // "a", "b"
    
    // misc
    VX_IR_OP_SHL, // "a", "b"
    VX_IR_OP_SHR, // "a", "b"

    // basic loop
    VX_IR_OP_FOR,      // "start": counter, "cond": (counter,States)->continue?, "stride": int, "do": (counter, States)->States, States
    VX_IR_OP_INFINITE, // "start": counter, "stride": int, "do": (counter, States)->States, States
    VX_IR_OP_WHILE,    // "cond": (States)->bool, "do": (counter)->States, States
    VX_IR_OP_CONTINUE,
    VX_IR_OP_BREAK,

    // advanced loop
    VX_IR_OP_FOREACH,        // "arr": array[T], "start": counter, "endEx": counter, "stride": int, "do": (T, States)->States, States
    VX_IR_OP_FOREACH_UNTIL,  // "arr": array[T], "start": counter, "cond": (T,States)->break?, "stride": int, "do": (T, States)->States, States
    VX_IR_OP_REPEAT,         // "start": counter, "endEx": counter, "stride": int, "do": (counter, States)->States, States
    VX_CIR_OP_CFOR,          // "start": ()->., "cond": ()->bool, "end": ()->., "do": (counter)->. 

    // conditional
    VX_IR_OP_IF,            // "cond": ()->bool, "then": ()->R, ("else": ()->R)
    VX_IR_OP_CMOV,          // "cond": ()->bool, "then": value, "else": value

    VX_LIR_OP_LABEL,        // "id"
    VX_LIR_GOTO,            // "id"
    VX_LIR_COND,            // "id", "cond": bool

    VX_IR_OP_CALL,          // "addr": int / fnref
    VX_IR_OP_TAILCALL,      // "addr": int / fnref
    VX_IR_OP_CONDTAILCALL,  // "addr": int / fnref, "cond": bool

    VX_IR_OP____END,
} vx_IrOpType;

bool vx_IrOp_ends_flow(vx_IrOp *op);

/** false for nop and label   true for everything else */
bool vx_IrOpType_has_effect(vx_IrOpType type);

void vx_IrOp_undeclare(vx_IrOp *op);

#define SSAOPTYPE_LEN (VX_IR_OP____END - VX_IR_OP_NOP)

extern const char *vx_IrOpType_names[SSAOPTYPE_LEN];

typedef struct {
    vx_IrVar var;
    vx_IrType *type;
} vx_IrTypedVar;

struct vx_IrOp_s {
    vx_IrTypedVar *outs;
    size_t         outs_len;

    vx_IrNamedValue *params;
    size_t           params_len;

    // TODO: TODO TODO MOST FNS IGNORE THAT!!!! BAD!!! (also rename and use in alternatives too)
    vx_IrValue  * states;
    size_t        states_len;

    vx_IrBlock  * parent;
    vx_IrOpType   id;

    vx_OpInfoList info;
};

void vx_IrOp_warn(vx_IrOp *op, const char *optMsg0, const char *optMsg1);

void vx_IrOp_dump(const vx_IrOp *op, FILE *out, size_t indent);

typedef struct {
    const vx_IrBlock *block;
    size_t start;
    size_t end;
} vx_IrView;

static vx_IrView vx_IrView_of_single(const vx_IrBlock *block, size_t index) {
    return (vx_IrView) {
        .block = block,
        .start = index,
        .end = index + 1,
    };
}
static vx_IrView vx_IrView_of_all(const vx_IrBlock *block) {
    return (vx_IrView) {
        .block = block,
        .start = 0,
        .end = block->ops_len,
    };
}
static size_t vx_IrView_len(const vx_IrView view) {
    return view.end - view.start;
}
/** returns true if found */
bool vx_IrView_find(vx_IrView *view, vx_IrOpType type);
vx_IrView vx_IrView_replace(vx_IrBlock *viewblock, vx_IrView view, const vx_IrOp *ops, size_t ops_len);
static vx_IrView vx_IrView_drop(const vx_IrView view, const size_t count) {
    vx_IrView out = view;
    out.block = view.block;
    out.start = view.start + count;
    if (out.start > out.end)
        out.start = out.end;
    return out;
}
static const vx_IrOp *vx_IrView_take(const vx_IrView view) {
    return &view.block->ops[view.start];
}
void vx_IrView_rename_var(vx_IrView view, vx_IrBlock *block, vx_IrVar old, vx_IrVar newv);
void vx_IrView_substitute_var(vx_IrView view, vx_IrBlock *block, vx_IrVar old, vx_IrValue newv);
static bool vx_IrView_has_next(const vx_IrView view) {
    return view.start < view.end;
}
void vx_IrView_deep_traverse(vx_IrView top, void (*callback)(vx_IrOp *op, void *data), void *data);

vx_IrValue *vx_IrOp_param(const vx_IrOp *op, vx_IrName name);

void vx_IrOp_init(vx_IrOp *op, vx_IrOpType type, vx_IrBlock *parent);
void vx_IrOp_add_out(vx_IrOp *op, vx_IrVar v, vx_IrType *t);
void vx_IrOp_add_param_s(vx_IrOp *op, vx_IrName name, vx_IrValue val);
void vx_IrOp_add_param(vx_IrOp *op, vx_IrNamedValue p);
static void vx_IrOp_steal_param(vx_IrOp *dest, const vx_IrOp *src, vx_IrName param) {
    vx_IrOp_add_param_s(dest, param, vx_IrValue_clone(*vx_IrOp_param(src, param)));
}
void vx_IrOp_remove_params(vx_IrOp *op);
void vx_IrOp_remove_out_at(vx_IrOp *op, size_t id);
void vx_IrOp_remove_param_at(vx_IrOp *op, size_t id);
void vx_IrOp_steal_outs(vx_IrOp *dest, const vx_IrOp *src);
void vx_IrOp_destroy(vx_IrOp *op);
void vx_IrOp_remove_state_at(vx_IrOp *op, size_t id);
bool vx_IrOp_is_volatile(vx_IrOp *op);
size_t vx_IrOp_inline_cost(vx_IrOp *op);
void vx_IrOp_steal_states(vx_IrOp *dest, const vx_IrOp *src);
bool vx_IrOp_is_tail(vx_IrOp *op);
vx_IrOp *vx_IrOp_next(vx_IrOp *op);

static void vx_IrBlock_root_set_var_decl(vx_IrBlock *root, vx_IrVar var, vx_IrOp *decl) {
    root->as_root.vars[var].decl_parent = decl->parent;
    root->as_root.vars[var].decl_idx = decl - decl->parent->ops;
}

static vx_IrOp *vx_IrBlock_root_get_var_decl(const vx_IrBlock *root, vx_IrVar var) {
    vx_IrBlock *parent = root->as_root.vars[var].decl_parent;
    if (parent == NULL)
        return NULL;
    size_t idx = root->as_root.vars[var].decl_idx;
    if (idx >= parent->ops_len)
        return NULL;
    return parent->ops + idx;
}

static void vx_IrBlock_root_set_label_decl(vx_IrBlock *root, size_t label , vx_IrOp *decl) {
    root->as_root.labels[label].decl_parent = decl->parent;
    root->as_root.labels[label].decl_idx = decl - decl->parent->ops;
}

static vx_IrOp *vx_IrBlock_root_get_label_decl(const vx_IrBlock *root, size_t label) {
    vx_IrBlock *parent = root->as_root.labels[label].decl_parent;
    if (parent == NULL)
        return NULL;
    size_t idx = root->as_root.labels[label].decl_idx;
    if (idx >= parent->ops_len)
        return NULL;
    return parent->ops + idx;
}

struct IrStaticIncrement {
    bool detected;
    vx_IrVar var;
    long long by;
};
struct IrStaticIncrement vx_IrOp_detect_static_increment(const vx_IrOp *op);

vx_IrOp *vx_IrBlock_inside_out_vardecl_before(const vx_IrBlock *block, vx_IrVar var, size_t before);
bool vx_IrBlock_is_volatile(const vx_IrBlock *block);
size_t vx_IrBlock_inline_cost(const vx_IrBlock *block);

#endif //IR_H
