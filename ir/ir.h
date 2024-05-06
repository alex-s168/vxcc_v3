#ifndef SSA_H
#define SSA_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#include "../common.h"

struct SsaOp_s;
typedef struct SsaOp_s SsaOp;

typedef size_t SsaVar;

typedef struct {
    SsaVar var;
    bool present;
} OptSsaVar;

#define SSAVAR_OPT_NONE (OptSsaVar) {.present = false,.var = 0}
#define SSAVAR_OPT_SOME(v) (OptSsaVar) {.present = true,.var = v}

typedef const char *SsaType;

struct SsaBlock_s;
typedef struct SsaBlock_s SsaBlock;

struct SsaBlock_s {
    SsaBlock *parent;
    size_t parent_index;

    bool is_root;
    struct {
        struct {
            SsaOp *decl;
        } *vars;
        size_t vars_len;
    } as_root;
    
    SsaVar *ins;
    size_t  ins_len;
    
    SsaOp  *ops;
    size_t  ops_len;
    
    SsaVar *outs;
    size_t  outs_len;

    bool should_free;
};

const SsaBlock *irblock_root(const SsaBlock *block);

VerifyErrors irblock_verify(const SsaBlock *block, OpPath path);

static int ir_verify(const SsaBlock *block) {
    OpPath path;
    path.ids = NULL;
    path.len = 0;
    const VerifyErrors errs = irblock_verify(block, path);
    verifyerrors_print(errs, stderr);
    verifyerrors_free(errs);
    return errs.len > 0;
}

/** DON'T RUN INIT AFTERWARDS */
SsaBlock *irblock_heapalloc(SsaBlock *parent, size_t parent_index);
void irblock_init(SsaBlock *block, SsaBlock *parent, size_t parent_index);
/** run AFTER you finished building it! */
void irblock_make_root(SsaBlock *block, size_t total_vars);
void irblock_add_in(SsaBlock *block, SsaVar var);
void irblock_add_op(SsaBlock *block, const SsaOp *op);
void irblock_add_all_op(SsaBlock *dest, const SsaBlock *src);
void irblock_add_out(SsaBlock *block, SsaVar out);
void irblock_destroy(SsaBlock *block);

SsaVar irblock_new_var(SsaBlock *block, SsaOp *decl);
void irblock_flatten(SsaBlock *block);
void irblock_swap_in_at(SsaBlock *block, size_t a, size_t b);
void irblock_swap_out_at(SsaBlock *block, size_t a, size_t b);
static void irblock_swap_state_at(SsaBlock *block, size_t a, size_t b) {
    irblock_swap_in_at(block, a, b);
    irblock_swap_out_at(block, a, b);
}
void irblock_remove_out_at(SsaBlock *block, size_t id);

bool irblock_var_used(const SsaBlock *block, SsaVar var);

void irblock_dump(const SsaBlock *block, FILE *out, size_t indent);

typedef struct {
    enum {
        SSA_VAL_IMM_INT,
        SSA_VAL_IMM_FLT,
        SSA_VAL_VAR,
        SSA_VAL_BLOCK,
    } type;

    union {
        long long imm_int;
        double imm_flt;
        SsaVar var;
        SsaBlock *block;
    };
} SsaValue;

SsaValue irvalue_clone(SsaValue value);
void irvalue_dump(SsaValue value, FILE *out, size_t indent);

SsaOp *irblock_finddecl_var(const SsaBlock *block, SsaVar var);
/** returns true if static eval ok; only touches dest if true */
bool irblock_staticeval_var(const SsaBlock *block, SsaVar var, SsaValue *dest);
bool irblock_mightbe_var(const SsaBlock *block, SsaVar var, SsaValue v);
bool irblock_alwaysis_var(const SsaBlock *block, SsaVar var, SsaValue v);
void irblock_staticeval(SsaBlock *block, SsaValue *v);

typedef enum {
    SSA_NAME_OPERAND_A,
    SSA_NAME_OPERAND_B,

    SSA_NAME_BLOCK,
    SSA_NAME_VALUE,
    SSA_NAME_COND,

    SSA_NAME_COND_THEN,
    SSA_NAME_COND_ELSE,

    SSA_NAME_LOOP_DO,
    SSA_NAME_LOOP_START,
    SSA_NAME_LOOP_ENDEX,
    SSA_NAME_LOOP_STRIDE,

    SSA_NAME_ALTERNATIVE_A,
    SSA_NAME_ALTERNATIVE_B,
} SsaName;

extern const char *irname_str[];

typedef struct {
    SsaName   name;
    SsaValue  val;
} SsaNamedValue;

static SsaNamedValue irnamedvalue_create(SsaName name, SsaValue v) {
    return (SsaNamedValue) {
        .name = name,
        .val = v,
    };
}
static SsaNamedValue irnamedvalue_clone(SsaNamedValue val) {
    return irnamedvalue_create(val.name, irvalue_clone(val.val));
}
void irnamedvalue_rename(SsaNamedValue *value, SsaName newn);
void irnamedvalue_destroy(SsaNamedValue v);

typedef enum {
    SSA_OP_NOP = 0,
    SSA_OP_IMM,            // "val"
    SSA_OP_FLATTEN_PLEASE, // "block"
    
    // convert
    /** for pointers only! */
    SSA_OP_REINTERPRET, // "val"
    SSA_OP_ZEROEXT,     // "val"
    SSA_OP_SIGNEXT,     // "val"
    SSA_OP_TOFLT,       // "val"
    SSA_OP_FROMFLT,     // "val"
    SSA_OP_BITCAST,     // "val"
        
    // arithm
    SSA_OP_ADD, // "a", "b"
    SSA_OP_SUB, // "a", "b"
    SSA_OP_MUL, // "a", "b"
    SSA_OP_DIV, // "a", "b"
    SSA_OP_MOD, // "a", "b"

    // compare
    SSA_OP_GT,  // "a", "b"
    SSA_OP_GTE, // "a", "b"
    SSA_OP_LT,  // "a", "b"
    SSA_OP_LTE, // "a", "b"
    SSA_OP_EQ,  // "a", "b"
    SSA_OP_NEQ, // "a", "b"

    // boolean
    SSA_OP_NOT, // "val"
    SSA_OP_AND, // "val"
    SSA_OP_OR,  // "val"

    // bitwise boolean
    SSA_OP_BITWISE_NOT, // "val"
    SSA_OP_BITWISE_AND, // "a", "b"
    SSA_OP_BITIWSE_OR,  // "a", "b"
    
    // misc
    SSA_OP_SHL, // "a", "b"
    SSA_OP_SHR, // "a", "b"

    // basic loop
    SSA_OP_FOR,      // "start": counter, "cond": (counter,States)->continue?, "stride": int, "do": (counter, States)->States, States
    SSA_OP_INFINITE, // "start": counter, "stride": int, "do": (counter, States)->States, States
    SSA_OP_WHILE,    // "cond": (States)->bool, "do": (counter)->States, States
    SSA_OP_CONTINUE,
    SSA_OP_BREAK,

    // advanced loop
    SSA_OP_FOREACH,        // "arr": array[T], "start": counter, "endEx": counter, "stride": int, "do": (T, States)->States, States
    SSA_OP_FOREACH_UNTIL,  // "arr": array[T], "start": counter, "cond": (T,States)->break?, "stride": int, "do": (T, States)->States, States
    SSA_OP_REPEAT,         // "start": counter, "endEx": counter, "stride": int, "do": (counter, States)->States, States
    CIR_OP_CFOR,           // "start": ()->., "cond": ()->bool, "end": ()->., "do": (counter)->.
    
    // conditional
    SSA_OP_IF,     // "cond": bool, "then": ()->R, ("else": ()->R)


    SSA_OP____END,
} SsaOpType;

#define SSAOPTYPE_LEN (SSA_OP____END - SSA_OP_NOP)

extern const char *iroptype_names[SSAOPTYPE_LEN];

typedef struct {
    SsaVar var;
    SsaType type;
} SsaTypedVar;

struct SsaOp_s {
    SsaType *types;
    size_t   types_len;

    SsaTypedVar *outs;
    size_t       outs_len;

    SsaNamedValue *params;
    size_t         params_len;

    SsaValue *states;
    size_t    states_len;

    SsaBlock *parent;
    SsaOpType id;
};

void irop_dump(const SsaOp *op, FILE *out, size_t indent);

typedef struct {
    const SsaBlock *block;
    size_t start;
    size_t end;
} SsaView;

static SsaView irview_of_single(const SsaBlock *block, size_t index) {
    return (SsaView) {
        .block = block,
        .start = index,
        .end = index + 1,
    };
}
static SsaView irview_of_all(const SsaBlock *block) {
    return (SsaView) {
        .block = block,
        .start = 0,
        .end = block->ops_len,
    };
}
static size_t irview_len(const SsaView view) {
    return view.end - view.start;
}
/** returns true if found */
bool irview_find(SsaView *view, SsaOpType type);
SsaView irview_replace(SsaBlock *viewblock, SsaView view, const SsaOp *ops, size_t ops_len);
static SsaView irview_drop(const SsaView view, const size_t count) {
    SsaView out = view;
    out.block = view.block;
    out.start = view.start + count;
    if (out.start > out.end)
        out.start = out.end;
    return out;
}
static const SsaOp *irview_take(const SsaView view) {
    return &view.block->ops[view.start];
}
void irview_rename_var(SsaView view, SsaBlock *block, SsaVar old, SsaVar newv);
void irview_substitude_var(SsaView view, SsaBlock *block, SsaVar old, SsaValue newv);
static bool irview_has_next(const SsaView view) {
    return view.start < view.end;
}
SsaOp *irblock_traverse(SsaView *current);
void irview_deep_traverse(SsaView top, void (*callback)(SsaOp *op, void *data), void *data);

SsaValue *irop_param(const SsaOp *op, SsaName name);

void irop_init(SsaOp *op, SsaOpType type, SsaBlock *parent);
void irop_add_type(SsaOp *op, SsaType type);
void irop_add_out(SsaOp *op, SsaVar v, SsaType t);
void irop_add_param_s(SsaOp *op, SsaName name, SsaValue val);
void irop_add_param(SsaOp *op, SsaNamedValue p);
static void irop_steal_param(SsaOp *dest, const SsaOp *src, SsaName param) {
    irop_add_param_s(dest, param, irvalue_clone(*irop_param(src, param)));
}
void irop_remove_params(SsaOp *op);
void irop_remove_out_at(SsaOp *op, size_t id);
void irop_remove_param_at(SsaOp *op, size_t id);
void irop_steal_outs(SsaOp *dest, const SsaOp *src);
void irop_destroy(SsaOp *op);
bool irop_anyparam_hasvar(SsaOp *op, SsaVar var);
void irop_remove_state_at(SsaOp *op, size_t id);
bool irop_is_pure(SsaOp *op);
void irop_steal_states(SsaOp *dest, const SsaOp *src);

struct SsaStaticIncrement {
    bool detected;
    SsaVar var;
    long long by;
};
struct SsaStaticIncrement irop_detect_static_increment(const SsaOp *op);

SsaOp *irblock_inside_out_vardecl_before(const SsaBlock *block, SsaVar var, size_t before);
bool irblock_is_pure(const SsaBlock *block);

#endif //SSA_H
