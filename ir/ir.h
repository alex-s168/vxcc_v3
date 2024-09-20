#ifndef IR_H
#define IR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "../common/common.h"
#include "../build/ir/ops.cdef.h"

// TODO: use kollektions
// TODO: add allocator similar to this to kallok (allibs) and use allocators here
// TODO make sure that all analysis functions iterate params AND args

typedef size_t vx_IrVar;

/** alloc things that are not meant to be freed or reallocated before end of compilation */
void * fastalloc(size_t bytes);
void * fastrealloc(void * old, size_t oldBytes, size_t newBytes);
void   fastfreeall(void);
static char * faststrdup(const char * str) {
    assert(str);
    size_t len = strlen(str) + 1;
    len *= sizeof(char);
    char * ret = (char*) fastalloc(len);
    assert(ret);
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

typedef struct {
    bool   sizeless;
    size_t size;
    size_t align;
    bool   isfloat;
} vx_IrTypeBase;

typedef struct {
    vx_IrType **args;
    size_t      args_len;

    vx_IrType  *nullableReturnType;
} vx_IrTypeFunc;

typedef enum {
    // present in: cir
    VX_IR_TYPE_KIND_CIR_STRUCT,

    // present in: cir, ssa
    VX_IR_TYPE_KIND_BASE,
    VX_IR_TYPE_FUNC,
} vx_IrTypeKind;

struct vx_IrType_s {
    const char *debugName;

    vx_IrTypeKind kind;

    union {
        vx_IrTypeBase       base;
        vx_IrTypeCIRStruct  cir_struct;
        vx_IrTypeFunc       func;
    };
};

static vx_IrType* vx_IrType_heap(void) {
    vx_IrType* ptr = (vx_IrType*) malloc(sizeof(vx_IrType));
    assert(ptr);
    memset(ptr, 0, sizeof(vx_IrType));
    return ptr;
}

size_t vx_IrType_size(vx_IrType *ty);
void vx_IrType_free(vx_IrType *ty);

static bool vx_IrType_compatible(vx_IrType *a, vx_IrType *b) {
    return a == b; // TODO (not used right now)
}

typedef struct {
    vx_IrVar var;
    vx_IrType *type;
} vx_IrTypedVar;

// TODO: make bit vec and re-name to alive_in_op
typedef struct {
    bool * used_in_op;
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

    vx_IrType ** ll_out_types;
    size_t       ll_out_types_len;

    bool should_free;

    const char *name;
};

typedef enum {
    VX_CU_BLOCK_IR,
    VX_CU_BLOCK_SYM_REF,
    VX_CU_BLOCK_ASM,
} vx_CUBlockType;

typedef struct {
    vx_CUBlockType type;
    union {
        vx_IrBlock * ir;
        const char * sym_ref;
        char *       asmb; // ptr to malloc()-ed asm source code
    } v;
    bool       do_export;
} vx_CUBlock;

// TODO: move opt config into vx_CU

/** single compilation unit */ 
typedef struct {
    vx_Target target;

    vx_CUBlock * blocks;
    size_t       blocks_len;
} vx_CU;

static vx_CUBlock* vx_CU_addBlock(vx_CU* vx_cu) {
    vx_cu->blocks = (vx_CUBlock*) realloc(vx_cu->blocks, sizeof(vx_CUBlock) * (vx_cu->blocks_len + 1));
    if (vx_cu->blocks == NULL) return NULL;
    return &vx_cu->blocks[vx_cu->blocks_len ++];
}

static void vx_CU_addIrBlock(vx_CU* vx_cu, vx_IrBlock* block, bool doExport) {
    vx_CUBlock* cb = vx_CU_addBlock(vx_cu);
    assert(cb);
    cb->type = VX_CU_BLOCK_IR;
    cb->v.ir = block;
    cb->do_export = doExport;
}

/** 0 if ok */
int vx_CU_compile(vx_CU * cu,
                  FILE* optionalOptimizedSsaIr,
                  FILE* optionalOptimizedLlIr,
                  FILE* optionalAsm,
                  vx_BinFormat optionalBinFormat, FILE* optionalBinOut);

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

    vx_IrType* no_read_rt_type;

    union {
        long long imm_int;
        double imm_flt;
        vx_IrVar var;

        vx_IrBlock *block;
        vx_IrType *ty;
        size_t id;
    };
} vx_IrValue;

#define VX_IR_VALUE_IMM_INT(varin) ((vx_IrValue) { .type = VX_IR_VAL_IMM_INT, .imm_int = varin })
#define VX_IR_VALUE_IMM_FLT(varin) ((vx_IrValue) { .type = VX_IR_VAL_IMM_FLT, .imm_flt = varin })
#define VX_IR_VALUE_VAR(varin)  ((vx_IrValue) { .type = VX_IR_VAL_VAR, .var = varin })
#define VX_IR_VALUE_UNINIT()    ((vx_IrValue) { .type = VX_IR_VAL_UNINIT })
#define VX_IR_VALUE_BLKREF(blk) ((vx_IrValue) { .type = VX_IR_VAL_BLOCKREF, .block = blk })
#define VX_IR_VALUE_TYPE(idin)  ((vx_IrValue) { .type = VX_IR_VAL_TYPE, .ty = idin })
#define VX_IR_VALUE_BLK(blk)    ((vx_IrValue) { .type = VX_IR_VAL_BLOCK, .block = blk })
#define VX_IR_VALUE_ID(idin)    ((vx_IrValue) { .type = VX_IR_VAL_ID, .id = idin })

void vx_IrValue_dump(vx_IrValue value, FILE *out, size_t indent);

/**
 * used for C IR transforms
 *
 * block is optional
 *
 * block:
 *   __  nested blocks can also exist
 *  /\
 *    \ search here
 *     \
 *   before
*/ 
vx_IrOp *vx_IrBlock_vardeclOutBefore(vx_IrBlock *block, vx_IrVar var, vx_IrOp *before);
vx_IrOp *vx_IrBlock_tail(vx_IrBlock *block);
/** DON'T RUN INIT AFTERWARDS */
vx_IrBlock *vx_IrBlock_initHeap(vx_IrBlock *parent, vx_IrOp *parent_op);
void vx_IrBlock_init(vx_IrBlock *block, vx_IrBlock *parent, vx_IrOp *parent_op);
/** run AFTER you finished building it! */
void vx_IrBlock_makeRoot(vx_IrBlock *block, size_t total_vars);
void vx_IrBlock_addIn(vx_IrBlock *block, vx_IrVar var, vx_IrType *type);
void vx_IrBlock_addOp(vx_IrBlock *block, const vx_IrOp *op);
/** WARNING: DON'T REF VARS IN OP THAT ARE NOT ALREADY INDEXED ROOT */
vx_IrOp *vx_IrBlock_addOpBuilding(vx_IrBlock *block);
vx_IrOp *vx_IrBlock_insertOpBuildingAfter(vx_IrOp *after);
void vx_IrBlock_addAllOp(vx_IrBlock *dest, const vx_IrBlock *src);
void vx_IrBlock_addOut(vx_IrBlock *block, vx_IrVar out);
void vx_IrBlock_destroy(vx_IrBlock *block);
vx_IrType *vx_IrBlock_typeofVar(vx_IrBlock *block, vx_IrVar var);
vx_IrVar vx_IrBlock_newVar(vx_IrBlock *block, vx_IrOp *decl);
size_t   vx_IrBlock_newLabel(vx_IrBlock *block, vx_IrOp *decl);
void vx_IrBlock_swapInAt(vx_IrBlock *block, size_t a, size_t b);
void vx_IrBlock_swapOutAt(vx_IrBlock *block, size_t a, size_t b);
void vx_IrBlock_removeOutAt(vx_IrBlock *block, size_t id);
size_t vx_IrBlock_appendLabelOp(vx_IrBlock *block);
/** for when you aquired a label id via vx_IrBlock_new_label(?, NULL) and want to add a label op at the tail*/
void vx_IrBlock_appendLabelOpPredefined(vx_IrBlock *block, size_t id);
void vx_IrBlock_putVar(vx_IrBlock *root, vx_IrVar var, vx_IrOp *decl);
void vx_IrBlock_putLabel(vx_IrBlock *root, size_t label, vx_IrOp *decl);
bool vx_IrBlock_anyPlaced(vx_IrBlock* block); // only for root blocks

static bool vx_IrBlock_empty(vx_IrBlock *block) {
    if (!block)
        return true;
    return block->first == NULL;
}

bool vx_IrBlock_varUsed(vx_IrBlock *block, vx_IrVar var);
void vx_IrBlock_dump(vx_IrBlock *block, FILE *out, size_t indent);
/** returns true if static eval ok; only touches dest if true */
bool vx_IrBlock_evalVar(vx_IrBlock *block, vx_IrVar var, vx_IrValue *dest);
bool vx_Irblock_mightbeVar(vx_IrBlock *block, vx_IrVar var, vx_IrValue v);
bool vx_Irblock_alwaysIsVar(vx_IrBlock *block, vx_IrVar var, vx_IrValue v);
void vx_Irblock_eval(vx_IrBlock *block, vx_IrValue *v);

/** can be ored together */ 
typedef enum { VX_RENAME_VAR_OUTPUTS = 0b1, VX_RENAME_VAR_INPUTS = 0b10, VX_RENAME_VAR_BOTH = 0b11 } vx_RenameVarCfg;
/** ret val: did rename any? **/
bool vx_IrBlock_renameVar(vx_IrBlock *block, vx_IrVar old, vx_IrVar newv, vx_RenameVarCfg cfg);
bool vx_IrBlock_deepTraverse(vx_IrBlock *block, bool (*callback)(vx_IrOp *op, void *data), void *data);
bool vx_IrBlock_vardeclIsInIns(vx_IrBlock *block, vx_IrVar var);
vx_IrOp* vx_IrBlock_varDeclRec(vx_IrBlock *block, vx_IrVar var);
vx_IrOp* vx_IrBlock_varDeclNoRec(vx_IrBlock *block, vx_IrVar var);
/** might not contain unique elemes */
vx_IrVar* vx_IrBlock_listDeclaredVarsRec(vx_IrBlock *block, size_t * listLenOut);

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

    VX_IR_NAME_IDX,
    VX_IR_NAME_STRUCT,
    VX_IR_NAME_TYPE,
    VX_IR_NAME_ELSIZE,
    VX_IR_NAME_OFF,

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
};

vx_IrOp* vx_IrBlock_lastOfType(vx_IrBlock* block, vx_IrOpType type);

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

#define FOR_INPUTS_REF(op,inp,fn) { \
    for (size_t __it = 0; __it < op->args_len; __it ++) {\
        vx_IrValue* inp = &op->args[__it]; \
        fn; \
    } \
    for (size_t __it = 0; __it < op->params_len; __it ++) { \
        vx_IrValue* inp = &op->params[__it].val; \
        fn; \
    } \
}

#define FOR_INPUTS(op,inp,fn) FOR_INPUTS_REF(op, __ref, { vx_IrValue inp = *__ref; fn; });

size_t vx_IrOp_countSuccessors(vx_IrOp *op);
static size_t vx_IrBlock_countOps(vx_IrBlock *block) {
    return block->first ? (vx_IrOp_countSuccessors(block->first) + 1) : 0;
}

vx_IrOp *vx_IrOp_predecessor(vx_IrOp *op);
void vx_IrOp_removeSuccessor(vx_IrOp *op);
/** should use remove_successor whenever possible! */
void vx_IrOp_remove(vx_IrOp *op);

/** false for nop and label   true for everything else */
bool vx_IrOpType_hasEffect(vx_IrOpType type);
bool vx_IrOp_endsFlow(vx_IrOp *op);
bool vx_IrOp_isVolatile(vx_IrOp *op);
bool vx_IrOp_hasSideEffect(vx_IrOp *op);
size_t vx_IrOp_inlineCost(vx_IrOp *op);
size_t vx_IrOp_execCost(vx_IrOp *op);

bool vx_IrBlock_endsFlow(vx_IrBlock *block);
bool vx_IrBlock_isVolatile(vx_IrBlock *block);
bool vx_IrBlock_hasSideEffect(vx_IrBlock *block);
size_t vx_IrBlock_inlineCost(vx_IrBlock *block);
size_t vx_IrBlock_execCost(vx_IrBlock *block);

void vx_IrOp_undeclare(vx_IrOp *op);
bool vx_IrOp_varUsed(const vx_IrOp *op, vx_IrVar var);
bool vx_IrOp_varInOuts(const vx_IrOp *op, vx_IrVar var);
void vx_IrOp_warn(vx_IrOp *op, const char *optMsg0, const char *optMsg1);
void vx_IrOp_dump(const vx_IrOp *op, FILE *out, size_t indent);
vx_IrValue *vx_IrOp_param(const vx_IrOp *op, vx_IrName name);
void vx_IrOp_init(vx_IrOp *op, vx_IrOpType type, vx_IrBlock *parent);
void vx_IrOp_addOut(vx_IrOp *op, vx_IrVar v, vx_IrType *t);
void vx_IrOp_addParam_s(vx_IrOp *op, vx_IrName name, vx_IrValue val);
void vx_IrOp_addParam(vx_IrOp *op, vx_IrNamedValue p);
void vx_IrOp_addArg(vx_IrOp *op, vx_IrValue val);
static void vx_IrOp_stealParam(vx_IrOp *dest, const vx_IrOp *src, vx_IrName param) {
    vx_IrValue* val = vx_IrOp_param(src, param);
    assert(val);
    vx_IrOp_addParam_s(dest, param, *val);
}
void vx_IrOp_removeParams(vx_IrOp *op);
void vx_IrOp_removeOutAt(vx_IrOp *op, size_t id);
void vx_IrOp_removeParamAt(vx_IrOp *op, size_t id);
void vx_IrOp_removeParam(vx_IrOp *op, vx_IrName name);
void vx_IrOp_stealOuts(vx_IrOp *dest, const vx_IrOp *src);
void vx_IrOp_destroy(vx_IrOp *op);
void vx_IrOp_removeArgAt(vx_IrOp *op, size_t id);
void vx_IrOp_stealArgs(vx_IrOp *dest, const vx_IrOp *src);
bool vx_IrOp_isTail(vx_IrOp *op);
bool vx_IrOp_after(vx_IrOp *op, vx_IrOp *after);
bool vx_IrOp_followingNoEffect(vx_IrOp* op);
vx_IrOp* vx_IrOp_nextWithEffect(vx_IrOp* op);

typedef bool (*vx_IrOpFilter)(vx_IrOp* op, void* userdata);

bool VX_IR_OPFILTER_ID__impl(vx_IrOp*,void*);
#define VX_IR_OPFILTER_ID(id) \
    VX_IR_OPFILTER_ID__impl, \
    (void*) (intptr_t) id

bool VX_IR_OPFILTER_COMPARISION__impl(vx_IrOp*,void*);
#define VX_IR_OPFILTER_COMPARISION \
    VX_IR_OPFILTER_COMPARISION__impl, \
    NULL 

bool VX_IR_OPFILTER_CONDITIONAL__impl(vx_IrOp*,void*);
#define VX_IR_OPFILTER_CONDITIONAL \
    VX_IR_OPFILTER_CONDITIONAL__impl, \
    NULL

bool VX_IR_OPFILTER_PURE__impl(vx_IrOp*,void*);
#define VX_IR_OPFILTER_PURE \
    VX_IR_OPFILTER_PURE__impl, \
    NULL

bool VX_IR_OPFILTER_BOTH__impl(vx_IrOp*,void*);
#define VX_IR_OPFILTER_BOTH(a, b) \
    VX_IR_OPFILTER_BOTH__impl, \
    (void*[]) { a, b }

bool vx_IrBlock_nextOpListBetween(vx_IrBlock* block,
                                  vx_IrOp** first, vx_IrOp** last,
                                  vx_IrOpFilter matchBegin, void *data0,
                                  vx_IrOpFilter matchEnd, void *data1);

static vx_IrOp* vx_IrOp_nextWhile(vx_IrOp* op, vx_IrOpFilter match, void *data0)
{
    while (op && match(op, data0)) {
        op = op->next;
    }
    return op;
}

static void vx_IrOp_earlierFirst(vx_IrOp** earlier, vx_IrOp** later)
{
    if (vx_IrOp_after(*earlier, *later)) {
        vx_IrOp* temp = *earlier;
        *earlier = *later;
        *later = temp;
    }
}

bool vx_IrBlock_allMatch(vx_IrOp* first, vx_IrOp* last,
                         vx_IrOpFilter fil, void* data);

bool vx_IrBlock_noneMatch(vx_IrOp* first, vx_IrOp* last,
                          vx_IrOpFilter fil, void* data);

bool vx_IrOp_inRange(vx_IrOp* op,
                     vx_IrOp* first, vx_IrOp* last);

bool vx_IrOp_allDepsInRangeOrArgs(vx_IrBlock* block, vx_IrOp* op,
                                  vx_IrOp* first, vx_IrOp* last);

struct IrStaticIncrement {
    bool detected;
    vx_IrVar var;
    long long by;
};
struct IrStaticIncrement vx_IrOp_detectStaticIncrement(vx_IrOp *op);

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
vx_IrTypeRef vx_IrValue_type(vx_IrBlock* root, vx_IrValue value);

static vx_IrTypeRef vx_ptrType(vx_IrBlock* root) {
    vx_IrType* type = vx_IrType_heap();
    type->kind = VX_IR_TYPE_KIND_BASE;
    type->debugName = "ptr";
    type->base.size = 8; // TODO 
    type->base.align = 0; //  ^^^ 
    type->base.isfloat = false;
    type->base.sizeless = false;
    return (vx_IrTypeRef) { .ptr = type, .shouldFree = true };
}

void vx_IrBlock_flattenFlattenPleaseRec(vx_IrBlock* block);

bool vx_IrBlock_llIsLeaf(vx_IrBlock* block);

#endif //IR_H
