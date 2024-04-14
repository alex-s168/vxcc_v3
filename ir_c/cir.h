#ifndef CIR_H
#define CIR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#include "../common.h"

struct CIROp_s;
typedef struct CIROp_s CIROp;

typedef size_t CIRVar;

typedef const char *CIRType;

struct CIRBlock_s;
typedef struct CIRBlock_s CIRBlock;

struct CIRBlock_s {
    CIRBlock *parent;
    size_t parent_index;

    bool is_root;
    struct {
        size_t vars_len;
    } as_root;

    CIRVar *ins;
    size_t  ins_len;

    CIROp  *ops;
    size_t  ops_len;

    CIRVar *outs;
    size_t  outs_len;

    bool should_free;
};

const CIRBlock *cirblock_root(const CIRBlock *block);

VerifyErrors cirblock_verify(const CIRBlock *block, OpPath path);

static int cir_verify(const CIRBlock *block) {
    OpPath path;
    path.ids = NULL;
    path.len = 0;
    const VerifyErrors errs = cirblock_verify(block, path);
    verifyerrors_print(errs, stderr);
    verifyerrors_free(errs);
    return errs.len > 0;
}

/** DON'T CALL cirblock_init AFTERWARDS! */
CIRBlock *cirblock_heapalloc(CIRBlock *parent, size_t parent_index);
void cirblock_init(CIRBlock *block, CIRBlock *parent, size_t parent_index);
/** run AFTER you finished building it! */
void cirblock_make_root(CIRBlock *block, size_t total_vars);
void cirblock_add_in(CIRBlock *block, CIRVar var);
void cirblock_add_op(CIRBlock *block, const CIROp *op);
void cirblock_add_all_op(CIRBlock *dest, const CIRBlock *src);
void cirblock_add_out(CIRBlock *block, CIRVar out);
void cirblock_destroy(CIRBlock *block);

void cirblock_normalize(CIRBlock *block);

typedef struct {
    enum {
        CIR_VAL_IMM_INT,
        CIR_VAL_IMM_FLT,
        CIR_VAL_VAR,
        CIR_VAL_BLOCK,
    } type;

    union {
        long long imm_int;
        double imm_flt;
        CIRVar var;
        CIRBlock *block;
    };
} CIRValue;

CIRVar cirblock_new_var(CIRBlock *block);
void cirblock_rename_var_after(CIRBlock *block, size_t i, CIRVar old, CIRVar newd);
static void cirblock_rename_var(CIRBlock *block, const CIRVar old, const CIRVar newd) {
    cirblock_rename_var_after(block, 0, old, newd);
}

typedef struct {
    char     *name;
    CIRValue  val;
} CIRNamedValue;

CIRNamedValue cirnamedvalue_create(const char *name, CIRValue v);
static CIRNamedValue cirnamedvalue_copy(CIRNamedValue val) {
    return cirnamedvalue_create(val.name, val.val);
}
void cirnamedvalue_destroy(CIRNamedValue v);

typedef enum {
    CIR_OP_NOP = 0,
    CIR_OP_IMM, // "val"
    CIR_OP_FLATTEN_PLEASE,  // "block": ()->.

    // convert
    /** for pointers only! */
    CIR_OP_REINTERPRET, // "val"
    CIR_OP_ZEROEXT,     // "val"
    CIR_OP_SIGNEXT,     // "val"
    CIR_OP_TOFLT,       // "val"
    CIR_OP_FROMFLT,     // "val"
    CIR_OP_BITCAST,     // "val"

    // arithm
    CIR_OP_ADD, // "a", "b"
    CIR_OP_SUB, // "a", "b"
    CIR_OP_MUL, // "a", "b"
    CIR_OP_DIV, // "a", "b"
    CIR_OP_MOD, // "a", "b"

    // compare
    CIR_OP_GT,  // "a", "b"
    CIR_OP_GTE, // "a", "b"
    CIR_OP_LT,  // "a", "b"
    CIR_OP_LTE, // "a", "b"
    CIR_OP_EQ,  // "a", "b"
    CIR_OP_NEQ, // "a", "b"

    // boolean
    CIR_OP_NOT, // "val"
    CIR_OP_AND, // "val"
    CIR_OP_OR,  // "val"

    // bitwise boolean
    CIR_OP_BITWISE_NOT, // "val"
    CIR_OP_BITWISE_AND, // "a", "b"
    CIR_OP_BITIWSE_OR,  // "a", "b"

    // misc
    CIR_OP_SHL, // "a", "b"
    CIR_OP_SHR, // "a", "b"

    // basic loop
    CIR_OP_FOR,      // "init": counter, "cond": (counter)->continue?, "stride": int, "do": (counter)->.
    CIR_OP_INFINITE, // "init": counter, "stride": int, "do": (counter)->.
    CIR_OP_WHILE,    // "cond": ()->bool, "do": (counter)->.
    CIR_OP_CONTINUE,
    CIR_OP_BREAK,
    CIR_OP_CFOR,     // "init": ()->., "cond": ()->bool, "end": ()->., "do": (counter)->.

    // advanced loop
    CIR_OP_FOREACH,        // "arr": array[T], "start": counter, "endEx": counter, "stride": int, "do": (T)->.
    CIR_OP_FOREACH_UNTIL,  // "arr": array[T], "start": counter, "cond": (T)->break?, "stride": int, "do": (T)->.
    CIR_OP_REPEAT,         // "start": counter, "endEx": counter, "stride": int, "do": (counter)->.

    // conditional
    CIR_OP_IF,     // "cond": bool, "then": ()->R, ("else": ()->R)


    CIR_OP____END,
} CIROpType;

#define CIROPTYPE_LEN (CIR_OP____END - CIR_OP_NOP)

typedef struct {
    CIRVar var;
    CIRType type;
} CIRTypedVar;

struct CIROp_s {
    CIRType *types;
    size_t   types_len;

    CIRTypedVar *outs;
    size_t       outs_len;

    CIROpType id;

    CIRNamedValue *params;
    size_t         params_len;

    CIRBlock *parent;
};

CIRValue *cirop_param(const CIROp *op, const char *name);

void cirop_init(CIROp *op, CIROpType type);
void cirop_add_type(CIROp *op, CIRType type);
void cirop_add_out(CIROp *op, CIRVar v, CIRType t);
void cirop_add_param_s(CIROp *op, const char *name, CIRValue val);
void cirop_add_param(CIROp *op, CIRNamedValue p);
static void cirop_steal_param(CIROp *dest, const CIROp *src, const char *param) {
    cirop_add_param_s(dest, param, *cirop_param(src, param));
}
void cirop_steal_all_params_starting_with(CIROp *dest, const CIROp *src, const char *start);
void cirop_remove_params(CIROp *op);
void cirop_steal_outs(CIROp *dest, const CIROp *src);
void cirop_destroy(CIROp *op);

CIROp *cirblock_inside_out_vardecl_before(const CIRBlock *block, CIRVar var, size_t before);

void cirblock_mksa_states(CIRBlock *block);
void cirblock_mksa_final(CIRBlock *block);

#endif //CIR_H
