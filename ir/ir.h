#ifndef IR_H
#define IR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>


#ifndef VX_BASE_TYPES
#define VX_BASE_TYPES
typedef size_t vx_IrVar;
#endif

#include "../common.h"
#include "../cg/cg.h"

/** alloc things that are not meant to be freed or reallocated before end of compilation */
void * fastalloc(size_t bytes);
void * fastrealloc(void * old, size_t oldBytes, size_t newBytes);
void   fastfreeall(void);
static char * faststrdup(const char * str) {
    size_t len = strlen(str) + 1;
    len *= sizeof(char);
    char * ret = (char*) fastalloc(len);
    memcpy(ret, str, len);
    return ret;
}

struct vx_IrOp_s;
typedef struct vx_IrOp_s vx_IrOp;

typedef struct {
    vx_IrVar var;
    bool present;
} vx_OptIrVar;

#define VX_IRVAR_OPT_NONE (vx_OptIrVar) {.present = false,.var = 0}
#define VX_IRVAR_OPT_SOME(v) (vx_OptIrVar) {.present = true,.var = v}

static const char * vx_OptIrVar_debug(vx_OptIrVar var) {
    if (var.present) {
        static char c[8];
        sprintf(c, "%zu", var.var);
        return c;
    }
    return "none";
}

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

typedef struct {
    vx_IrType **args;
    size_t      args_len;

    vx_IrType  *nullableReturnType;
} vx_IrTypeFunc;

struct vx_IrType_s {
    const char *debugName;

    enum {
        // present in: cir
        VX_IR_TYPE_KIND_CIR_STRUCT,
        VX_IR_TYPE_KIND_CIR_UNION,

        // present in: cir, ssa
        VX_IR_TYPE_KIND_BASE,
        VX_IR_TYPE_FUNC,
    } kind;

    union {
        vx_IrTypeBase       base;
        vx_IrTypeCIRStruct  cir_struct;
        vx_IrTypeCIRUnion   cir_union;
        vx_IrTypeFunc       func;
    };
};

static vx_IrType* vx_IrType_heap(void) {
    return (vx_IrType*) memset(malloc(sizeof(vx_IrType)), 0, sizeof(vx_IrType));
}

#define PTRSIZE (8)

// TODO: remove cir checks and make sure fn called after cir type expand & MAKE TYPE EXPAND AWARE OF MEMBER ALIGN FOR UNIONS
static size_t vx_IrType_size(vx_IrType *ty) {
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
    }
}

static void vx_IrType_free(vx_IrType *ty) {
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
            bool ever_placed;
            /** 0 is none! */
            vx_CgReg  reg; 
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

    const char *name;
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
        VX_IR_VAL_BLOCKREF,

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
vx_IrOp *vx_IrBlock_vardecl_out_before(vx_IrBlock *block, vx_IrVar var, vx_IrOp *before);
vx_IrOp *vx_IrBlock_tail(vx_IrBlock *block);
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
void vx_IrBlock_putVar(vx_IrBlock *root, vx_IrVar var, vx_IrOp *decl);

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
bool vx_IrBlock_vardecl_is_in_ins(vx_IrBlock *block, vx_IrVar var);

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

// TODO: negate
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
    VX_IR_OP_AND, // "a", "b"
    VX_IR_OP_OR,  // "a", "b"

    // bitwise boolean
    VX_IR_OP_BITWISE_NOT, // "val"
    VX_IR_OP_BITWISE_AND, // "a", "b"
    VX_IR_OP_BITIWSE_OR,  // "a", "b"
    
    // misc
    VX_IR_OP_SHL, // "a", "b"
    VX_IR_OP_SHR, // "a", "b"  // TODO: ASHR

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

    VX_IR_OP_VSCALE,        // "len": int, "elsize": int, "fn": (vscale)->Rets   -> Rets 

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

#define MKARR(...) { __VA_ARGS__ }

#define FOR_PARAMS(op,want,paramn,fn) { \
    vx_IrName __names[] = want; \
    for (size_t __it = 0; __it < sizeof(__names) / sizeof(vx_IrName); __it ++) { \
        vx_IrValue *__param = vx_IrOp_param(op, __names[__it]); \
        if (__param) {\
            vx_IrValue paramn = *__param; \
            fn; \
        }\
    } \
}

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
void vx_IrOp_remove_param(vx_IrOp *op, vx_IrName name);
void vx_IrOp_steal_outs(vx_IrOp *dest, const vx_IrOp *src);
void vx_IrOp_destroy(vx_IrOp *op);
void vx_IrOp_remove_state_at(vx_IrOp *op, size_t id);
bool vx_IrOp_is_volatile(vx_IrOp *op);
size_t vx_IrOp_inline_cost(vx_IrOp *op);
void vx_IrOp_steal_states(vx_IrOp *dest, const vx_IrOp *src);
bool vx_IrOp_is_tail(vx_IrOp *op);
bool vx_IrOp_after(vx_IrOp *op, vx_IrOp *after);

struct IrStaticIncrement {
    bool detected;
    vx_IrVar var;
    long long by;
};
struct IrStaticIncrement vx_IrOp_detect_static_increment(vx_IrOp *op);

typedef struct {
    vx_IrType *ptr;
    bool shouldFree;
} vx_IrTypeRef;

static void vx_IrTypeRef_drop(vx_IrTypeRef ref) {
    if (ref.shouldFree) {
        vx_IrType_free(ref.ptr);
    }
}

vx_IrTypeRef vx_IrBlock_type(vx_IrBlock* block);

// null if depends on context or has no type 
static vx_IrTypeRef vx_IrValue_type(vx_IrBlock* root, vx_IrValue value) {
    switch (value.type) {
        case VX_IR_VAL_IMM_INT:
        case VX_IR_VAL_IMM_FLT:
        case VX_IR_VAL_UNINIT:
        case VX_IR_VAL_TYPE:
        case VX_IR_VAL_ID:
        case VX_IR_VAL_BLOCK:
            return (vx_IrTypeRef) { .ptr = NULL, .shouldFree = false };

        case VX_IR_VAL_BLOCKREF:
            return vx_IrBlock_type(value.block);

        case VX_IR_VAL_VAR:
            return (vx_IrTypeRef) { .ptr = vx_IrBlock_typeof_var(root, value.var), .shouldFree = false };
    }
}

#endif //IR_H
