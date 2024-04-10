#ifndef SSA_H
#define SSA_H

#include <stddef.h>
#include <stdbool.h>

struct SsaOp_s;
typedef struct SsaOp_s SsaOp;

typedef size_t SsaVar;

typedef const char *SsaType;

struct SsaBlock_s;
typedef struct SsaBlock_s SsaBlock;

struct SsaBlock_s {
    SsaBlock *parent;
    
    SsaVar *ins;
    size_t  ins_len;
    
    SsaOp  *ops;
    size_t  ops_len;
    
    SsaVar *outs;
    size_t  outs_len;
};

void ssablock_init(SsaBlock *block, SsaBlock *parent);
void ssablock_add_in(SsaBlock *block, SsaVar var);
void ssablock_add_op(SsaBlock *block, const SsaOp *op);
void ssablock_add_out(SsaBlock *block, SsaVar out);
void ssablock_destroy(SsaBlock *block);

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

SsaOp *ssablock_finddecl_var(const SsaBlock *block, SsaVar var);
void ssablock_rename_var(SsaBlock *block, SsaVar old, SsaVar new);
/** returns true if static eval ok */
bool ssablock_staticeval_var(const SsaBlock *block, SsaVar var, SsaValue *dest);
bool ssablock_mightbe_var(const SsaBlock *block, SsaVar var, SsaValue v);
bool ssablock_alwaysis_var(const SsaBlock *block, SsaVar var, SsaValue v);

typedef struct {
    char     *name;
    SsaValue  val;
} SsaNamedValue;

SsaNamedValue ssanamedvalue_create(const char *name, SsaValue v);
static SsaNamedValue ssanamedvalue_copy(SsaNamedValue val) {
    return ssanamedvalue_create(val.name, val.val);
}
void ssanamedvalue_destroy(SsaNamedValue v);

typedef enum {
    SSA_OP_IMM, // "val"
    
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

    // basic loop
    SSA_OP_FOR,      // "init": counter, "cond": (counter)->continue?, "do": (counter)->.
    SSA_OP_INFINITE, // "init": counter, "do": (counter)->.
    SSA_OP_CONTINUE,
    SSA_OP_BREAK,

    // advanced loop
    SSA_OP_FOREACH,        // "arr": array[T], "start": counter, "endEx": counter, "do": (T)->.
    SSA_OP_FOREACH_UNTIL,  // "arr": array[T], "start": counter, "cond": (T)->break?, "do": (T)->.
    SSA_OP_REPEAT,         // "start": counter, "endEx": counter, "do": (counter)->.
} SsaOpType;

typedef struct {
    SsaVar var;
    SsaType type;
} SsaTypedVar;

struct SsaOp_s {
    SsaType *types;
    size_t   types_len;

    SsaTypedVar *outs;
    size_t       outs_len;
    
    SsaOpType id;

    SsaNamedValue *params;
    size_t         params_len;
};

typedef struct {
    const SsaBlock *block;
    size_t start;
    size_t end;
} SsaView;

static SsaView ssaview_of_single(const SsaBlock *block, size_t index) {
    return (SsaView) {
        .block = block,
        .start = index,
        .end = index + 1,
    };
}
static SsaView ssaview_of_all(const SsaBlock *block) {
    return (SsaView) {
        .block = block,
        .start = 0,
        .end = block->ops_len,
    };
}
static size_t ssaview_len(const SsaView view) {
    return view.end - view.start;
}
/** returns true if found */
bool ssaview_find(SsaView *view, SsaOpType type);
SsaView ssaview_replace(SsaBlock *viewblock, SsaView view, const SsaOp *ops, size_t ops_len);
static SsaView ssaview_drop(const SsaView view, const size_t count) {
    SsaView new = view;
    new.block = view.block;
    new.start = view.start + count;
    if (new.start > new.end)
        new.start = new.end;
    return new;
}
static const SsaOp *ssaview_take(const SsaView view) {
    return &view.block->ops[view.start];
}
void ssaview_rename_var(SsaView view, SsaBlock *block, SsaVar old, SsaVar new);

SsaValue *ssaop_param(const SsaOp *op, const char *name);

void ssaop_init(SsaOp *op, SsaOpType type);
void ssaop_add_type(SsaOp *op, SsaType type);
void ssaop_add_out(SsaOp *op, SsaVar v, SsaType t);
void ssaop_add_param_s(SsaOp *op, const char *name, SsaValue val);
void ssaop_add_param(SsaOp *op, SsaNamedValue p);
static void ssaop_steal_param(SsaOp *dest, const SsaOp *src, const char *param) {
    ssaop_add_param_s(dest, param, *ssaop_param(src, param));
}
void ssaop_destroy(SsaOp *op);

#endif //SSA_H
