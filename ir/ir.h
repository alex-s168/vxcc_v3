#ifndef IR_H
#define IR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TOOD: conditional tailcall & use in ssa->ll lowering


/** alloc things that are not meant to be freed or reallocated before end of compilation */
void * fastalloc(size_t bytes);

void fastfreeall(void);



#include "../common.h"
#include "../cg/cg.h"

typedef enum {
    VX_SEL_ONE_OF,
    VX_SEL_NONE_OF,
    VX_SEL_ANY,
    VX_SEL_NONE,
} vx_SelKind;

typedef size_t vx_RegRef;

typedef struct {
    vx_RegRef *items;
    size_t     count;
} vx_RegRefList;

/** uses fastalloc!! */
vx_RegRefList vx_RegRefList_fixed(size_t count);
bool vx_RegRefList_contains(vx_RegRefList list, vx_RegRef reg);
vx_RegRefList vx_RegRefList_intersect(vx_RegRefList a, vx_RegRefList b);
vx_RegRefList vx_RegRefList_union(vx_RegRefList a, vx_RegRefList b);
vx_RegRefList vx_RegRefList_remove(vx_RegRefList a, vx_RegRefList rem);

typedef struct {
    bool or_stack;
    bool or_mem;
    vx_SelKind kind;
    vx_RegRefList value;
} vx_RegAllocConstraint;

bool vx_RegAllocConstraint_matches(vx_RegAllocConstraint constraint, vx_RegRef reg);
vx_RegAllocConstraint vx_RegAllocConstraint_merge(vx_RegAllocConstraint a, vx_RegAllocConstraint b);

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
    vx_IrVar var;
    vx_IrType *type;
} vx_IrTypedVar;

typedef struct {
    vx_IrOp *first;
    vx_IrOp *last;
} lifetime;

struct vx_IrBlock_s;
typedef struct vx_IrBlock_s vx_IrBlock;

struct vx_IrBlock_s {
    vx_IrBlock *parent;
    vx_IrOp    *parent_op;

    bool is_root;
    struct {
        struct {
            vx_IrOp *decl;

            lifetime  ll_lifetime;
            vx_IrType *ll_type;

            vx_RegAllocConstraint cg_regconstraint;
        } *vars;
        size_t vars_len;

        struct {
            vx_IrOp *decl;
        } *labels;
        size_t labels_len;
    } as_root;

    vx_IrTypedVar *ins;
    size_t    ins_len;

    vx_IrOp *first;

    vx_IrVar *outs;
    size_t    outs_len;

    bool should_free;
};

vx_IrBlock *vx_IrBlock_root(vx_IrBlock *block);

// TODO: do differently

vx_Errors vx_IrBlock_verify(vx_IrBlock *block);

static int vx_ir_verify(vx_IrBlock *block) {
    const vx_Errors errs = vx_IrBlock_verify(block);
    vx_Errors_print(errs, stderr);
    vx_Errors_free(errs);
    return errs.len > 0;
}

// TODO: move builder functions into separate header

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

void vx_IrValue_dump(vx_IrValue value, FILE *out, size_t indent);

/** DON'T RUN INIT AFTERWARDS */
vx_IrBlock *vx_IrBlock_init_heap(vx_IrBlock *parent, vx_IrOp *parent_op);
void vx_IrBlock_init(vx_IrBlock *block, vx_IrBlock *parent, vx_IrOp *parent_op);
/** run AFTER you finished building it! */
void vx_IrBlock_make_root(vx_IrBlock *block, size_t total_vars);
void vx_IrBlock_add_in(vx_IrBlock *block, vx_IrVar var, vx_IrType *type);
void vx_IrBlock_add_op(vx_IrBlock *block, const vx_IrOp *op);
/** WARNING: DON'T REF VARS IN OP THAT ARE NOT ALREADY INDEXED ROOT */
vx_IrOp *vx_IrBlock_add_op_building(vx_IrBlock *block);
vx_IrOp *vx_IrBlock_insert_op_building_after(vx_IrOp *after);
void vx_IrBlock_add_all_op(vx_IrBlock *dest, const vx_IrBlock *src);
void vx_IrBlock_add_out(vx_IrBlock *block, vx_IrVar out);
void vx_IrBlock_destroy(vx_IrBlock *block);
vx_IrType *vx_IrBlock_typeof_var(vx_IrBlock *block, vx_IrVar var);
vx_IrVar vx_IrBlock_new_var(vx_IrBlock *block, vx_IrOp *decl);
size_t   vx_IrBlock_new_label(vx_IrBlock *block, vx_IrOp *decl);
void vx_IrBlock_swap_in_at(vx_IrBlock *block, size_t a, size_t b);
void vx_IrBlock_swap_out_at(vx_IrBlock *block, size_t a, size_t b);
void vx_IrBlock_remove_out_at(vx_IrBlock *block, size_t id);
size_t vx_IrBlock_append_label_op(vx_IrBlock *block);

static bool vx_IrBlock_empty(vx_IrBlock *block) {
    if (!block)
        return true;
    return block->first == NULL;
}

bool vx_IrBlock_var_used(vx_IrBlock *block, vx_IrVar var);
void vx_IrBlock_dump(vx_IrBlock *block, FILE *out, size_t indent);
/** returns true if static eval ok; only touches dest if true */
bool vx_IrBlock_eval_var(vx_IrBlock *block, vx_IrVar var, vx_IrValue *dest);
bool vx_Irblock_mightbe_var(vx_IrBlock *block, vx_IrVar var, vx_IrValue v);
bool vx_Irblock_alwaysis_var(vx_IrBlock *block, vx_IrVar var, vx_IrValue v);
void vx_Irblock_eval(vx_IrBlock *block, vx_IrValue *v);
void vx_IrBlock_rename_var(vx_IrBlock *block, vx_IrVar old, vx_IrVar newv);
void vx_IrBlock_substitute_var(vx_IrBlock *block, vx_IrVar old, vx_IrValue newv);
bool vx_IrBlock_deep_traverse(vx_IrBlock *block, bool (*callback)(vx_IrOp *op, void *data), void *data);
bool vx_IrBlock_is_volatile(vx_IrBlock *block);
size_t vx_IrBlock_inline_cost(vx_IrBlock *block);

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
    VX_IR_OP_IMM = 0,        // "val"
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

#define SSAOPTYPE_LEN (VX_IR_OP____END - VX_IR_OP_IMM)

extern const char *vx_IrOpType_names[SSAOPTYPE_LEN];

struct vx_IrOp_s {
    vx_IrOp *next;

    vx_IrTypedVar *outs;
    size_t         outs_len;

    vx_IrNamedValue *params;
    size_t           params_len;

    vx_IrValue  * args;
    size_t        args_len;

    vx_IrBlock  * parent;
    vx_IrOpType   id;

    vx_OpInfoList info;
};

vx_IrOp *vx_IrOp_predecessor(vx_IrOp *op);
void vx_IrOp_remove_successor(vx_IrOp *op);
/** should use remove_successor whenever possible! */
void vx_IrOp_remove(vx_IrOp *op);
bool vx_IrOp_ends_flow(vx_IrOp *op);
/** false for nop and label   true for everything else */
bool vx_IrOpType_has_effect(vx_IrOpType type);
void vx_IrOp_undeclare(vx_IrOp *op);
bool vx_IrOp_var_used(const vx_IrOp *op, vx_IrVar var);
void vx_IrOp_warn(vx_IrOp *op, const char *optMsg0, const char *optMsg1);
void vx_IrOp_dump(const vx_IrOp *op, FILE *out, size_t indent);
vx_IrValue *vx_IrOp_param(const vx_IrOp *op, vx_IrName name);
void vx_IrOp_init(vx_IrOp *op, vx_IrOpType type, vx_IrBlock *parent);
void vx_IrOp_add_out(vx_IrOp *op, vx_IrVar v, vx_IrType *t);
void vx_IrOp_add_param_s(vx_IrOp *op, vx_IrName name, vx_IrValue val);
void vx_IrOp_add_param(vx_IrOp *op, vx_IrNamedValue p);
static void vx_IrOp_steal_param(vx_IrOp *dest, const vx_IrOp *src, vx_IrName param) {
    vx_IrOp_add_param_s(dest, param, *vx_IrOp_param(src, param));
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

struct IrStaticIncrement {
    bool detected;
    vx_IrVar var;
    long long by;
};
struct IrStaticIncrement vx_IrOp_detect_static_increment(vx_IrOp *op);

#endif //IR_H
