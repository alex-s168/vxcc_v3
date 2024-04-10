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
    SSA_OP_IMM,
    
    // convert
    /** for pointers only! */ SSA_OP_REINTERPRET,
    SSA_OP_ZEROEXT,
    SSA_OP_SIGNEXT,
    SSA_OP_TOFLT,
    SSA_OP_FROMFLT,
    SSA_OP_BITCAST,
        
    // arithm
    SSA_OP_ADD,
    SSA_OP_SUB,
    SSA_OP_MUL,
    SSA_OP_DIV,

    // compare
    SSA_OP_GT,
    SSA_OP_GTE,
    SSA_OP_LT,
    SSA_OP_LTE,
    SSA_OP_EQ,
    SSA_OP_NEQ,

    // boolean
    SSA_OP_NOT,
    SSA_OP_AND,
    SSA_OP_OR,

    // bitwise boolean
    SSA_OP_BITWISE_NOT,
    SSA_OP_BITWISE_AND,
    SSA_OP_BITIWSE_OR,

    // basic loop
    SSA_OP_FOR,
    SSA_OP_INFINITE,
    SSA_OP_CONTINUE,
    SSA_OP_BREAK,

    // advanced loop
    SSA_OP_FOREACH,
    SSA_OP_FOREACH_UNTIL,
    SSA_OP_REPEAT,
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
void ssaop_destroy(SsaOp *op);

#endif //SSA_H
