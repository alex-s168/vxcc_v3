#include "cg.h"
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

// TODO: use lea for mul sometimes 
// TODO: mul / div by power of 2
// TODO: no tailcall opt if epilog

struct Location;

typedef struct {
    size_t id;
    const char * name[7];

    struct Location * stored;
} RegType;

#define MkRegType(idi, name0, name1, name2, name3) \
    ((RegType) { .id = (idi), .name = { name0, name1, name2, name3 }, .stored = NULL })
#define MkRegType7(idi, name0, name1, name2, name3, name4, name5, name6) \
    ((RegType) { .id = (idi), .name = { name0, name1, name2, name3, name4, name5, name6 }, .stored = NULL })
#define MkVecRegTy(id, vid) \
    MkRegType7((id), "xmm" #vid, "xmm" #vid, "xmm" #vid, "xmm" #vid, "xmm" #vid, "ymm" #vid, "zmm" #vid)

static RegType REG_RAX  = MkRegType(0,  "al",   "ax",   "eax",  "rax");
static RegType REG_RBX  = MkRegType(1,  "bl",   "bx",   "ebx",  "rbx");
static RegType REG_RCX  = MkRegType(2,  "cl",   "cx",   "ecx",  "rcx");
static RegType REG_RSP  = MkRegType(3,  "spl",  "sp",   "esp",  "rsp");
static RegType REG_RBP  = MkRegType(4,  "bpl",  "bp",   "ebp",  "rbp");
static RegType REG_RDX  = MkRegType(5,  "dl",   "dx",   "edx",  "rdx");
static RegType REG_RSI  = MkRegType(6,  "sil",  "si",   "esi",  "rsi");
static RegType REG_RDI  = MkRegType(7,  "dil",  "di",   "edi",  "rdi");
static RegType REG_R8   = MkRegType(8,  "r8b",  "r8w",  "r8d",  "r8" );
static RegType REG_R9   = MkRegType(9,  "r9b",  "r9w",  "r9d",  "r9" );
static RegType REG_R10  = MkRegType(10, "r10b", "r10w", "r10d", "r10");
static RegType REG_R11  = MkRegType(11, "r11b", "r11w", "r11d", "r11");
static RegType REG_R12  = MkRegType(12, "r12b", "r12w", "r12d", "r12");
static RegType REG_R13  = MkRegType(13, "r13b", "r13w", "r13d", "r13");
static RegType REG_R14  = MkRegType(14, "r14b", "r14w", "r14d", "r14");
static RegType REG_R15  = MkRegType(15, "r15b", "r15w", "r15d", "r15");
#define IntRegCount (16)

static RegType REG_XMM0 = MkVecRegTy(16, 0);
static RegType REG_XMM1 = MkVecRegTy(17, 1);
static RegType REG_XMM2 = MkVecRegTy(18, 2);
static RegType REG_XMM3 = MkVecRegTy(19, 3);
static RegType REG_XMM4 = MkVecRegTy(20, 4);
static RegType REG_XMM5 = MkVecRegTy(21, 5);
static RegType REG_XMM6 = MkVecRegTy(22, 6);
static RegType REG_XMM7 = MkVecRegTy(23, 7);

static RegType REG_XMM8 = MkVecRegTy(24, 8); 
static RegType REG_XMM9 = MkVecRegTy(25, 9);

#define RegCount (26)

#define IsIntReg(rid) (rid < IntRegCount)
#define VecRegId(rid) (rid - IntRegCount)

static RegType* RegLut[RegCount] = {
    &REG_RAX, &REG_RBX, &REG_RCX, &REG_RSP, &REG_RBP, &REG_RDX, &REG_RSI, &REG_RDI,
    &REG_R8,  &REG_R9,  &REG_R10, &REG_R11, &REG_R12, &REG_R13, &REG_R14, &REG_R15,

    &REG_XMM0, &REG_XMM1, &REG_XMM2, &REG_XMM3,
    &REG_XMM4, &REG_XMM5, &REG_XMM6, &REG_XMM7,
};

typedef enum {
    LOC_REG = 0,
    LOC_EA,
    LOC_MEM,
    LOC_IMM,
    LOC_LABEL,
    LOC_INVALID,
} LocationType;

#define NEGATIVE (-1)
#define POSITIVE (1)

typedef struct Location {
    size_t bytesWidth;
    LocationType type;
    union {
        struct {
            int64_t bits;
        } imm;

        struct {
            size_t id;
        } reg;

        struct {
            struct Location* base;
            int  offsetSign;
            struct Location* offset; // nullable!
            size_t offsetMul;
        } ea;

        struct {
            const char * label;
        } label;

        struct {
            struct Location* address;
        } mem;
    } v;
} Location;

#define LocImm(width, value) \
    ((Location) { .bytesWidth = (width), .type = LOC_IMM, .v.imm = (value) })
#define LocReg(width, regId) \
    ((Location) { .bytesWidth = (width), .type = LOC_REG, .v.reg = (regId) })
#define LocEA(width, basee, off, sign, mul) \
    ((Location) { .bytesWidth = (width), .type = LOC_EA, .v.ea.base = (basee), .v.ea.offsetSign = (sign), .v.ea.offset = (off), .v.ea.offsetMul = (mul) })
#define LocMem(width, eaa) \
    ((Location) { .bytesWidth = (width), .type = LOC_MEM, .v.mem.address = (eaa) })
#define LocLabel(str) \
    ((Location) { .type = LOC_LABEL, .v.label.label = (str) })

Location* loc_opt_copy(Location* old) {
    Location* new = fastalloc(sizeof(Location));
    switch (old->type) {
        case LOC_REG: { 
            *new = LocReg(old->bytesWidth, old->v.reg.id);
            return new;
        }

        case LOC_IMM: {
            *new = LocImm(old->bytesWidth, old->v.imm.bits);
            return new;
        }

        case LOC_EA: {
            if (old->v.ea.offsetMul == 0) {
                return loc_opt_copy(old);
            }

            *new = LocEA(old->bytesWidth, old->v.ea.base, old->v.ea.offset, old->v.ea.offsetSign, old->v.ea.offsetMul);
            return new;
        }

        case LOC_MEM: {
            *new = LocMem(old->bytesWidth, old->v.mem.address);
            return new;
        }

        case LOC_LABEL: {
            *new = LocLabel(old->v.label.label);
            return new;
        }

        case LOC_INVALID:
	default: {
            assert(false);
            return NULL;
        }
    }
}

static char widthToWidthWidth(char width) {
    static char widthToWidthWidthLut[] = {
        [0] = 0,
        [1] = 1,
        [2] = 2,
        [3] = 4,
        [4] = 4,
        [5] = 8,
        [6] = 8,
        [7] = 8,
        [8] = 8,
    };

    if (width > 8) {
        if (width > 16) {
            if (width > 32) { 
                assert(width <= 64);
                return 64;
            }
            return 32;
        }
        return 16;
    }
    return widthToWidthWidthLut[width];
}

static char widthToWidthIdx(char width) {
    static char lut[] = {
        [0] = 0,
        [1] = 0,
        [2] = 1,
        [3] = 2,
        [4] = 2,
        [5] = 3,
        [6] = 3,
        [7] = 3,
        [8] = 3,
    };


    if (width > 8) {
        if (width > 16) {
            if (width > 32) { 
                assert(width <= 64);
                return 6;
            }
            return 5;
        }
        return 4;
    }

    return lut[width];
}

static const char * widthWidthToASM[] = {
    [0] = "wtf",
    [1] = "byte",
    [2] = "word",
    [4] = "dword",
    [8] = "qword",
};

static bool needEpilog;

static void emit(Location*, FILE*);

static void emitEa(Location* ea, FILE* out) {
    assert(ea->v.ea.base->type != LOC_MEM);
    assert(ea->v.ea.base->type != LOC_EA);

    emit(ea->v.ea.base, out);
    if (ea->v.ea.offset != NULL) {
        assert(ea->v.ea.offset->type != LOC_MEM);
        assert(ea->v.ea.offset->type != LOC_EA);

        if (ea->v.ea.offsetSign == NEGATIVE) {
            fputs(" - ", out);
        } else {
            fputs(" + ", out);
        }
        emit(ea->v.ea.offset, out);
        fprintf(out, " * %zu", ea->v.ea.offsetMul);
    }
}

static void emit(Location* loc, FILE* out) {
    switch (loc->type) {
        case LOC_EA:
            emitEa(loc, out);
            break;

        case LOC_IMM:
            fprintf(out, "%zu", loc->v.imm.bits);
            break;

        case LOC_MEM: {
            char ww = widthToWidthWidth(loc->bytesWidth);
            const char * str = widthWidthToASM[ww];
            fputs(str, out);
            fputs(" [", out);
            emit(loc->v.mem.address, out);
            fputs("]", out);
        } break;

        case LOC_REG:
            fputs(RegLut[loc->v.reg.id]->name[widthToWidthIdx(loc->bytesWidth)], out);
            break;

        case LOC_LABEL:
            fputs(loc->v.label.label, out);
            break;

        case LOC_INVALID:
            assert(false);
            break;
    }
}

static Location* gen_imm_var(size_t size, int64_t bits) {
    Location* l = fastalloc(sizeof(Location));
    *l = LocImm(size, bits);
    return l;
}

static Location* gen_reg_var(size_t size, size_t regId) {
    Location* l = fastalloc(sizeof(Location));
    *l = LocReg(size, regId);
    return l;
}

static Location* gen_mem_var(size_t size, Location* addr) {
    Location* l = fastalloc(sizeof(Location));
    *l = LocMem(size, addr);
    return l;
}

static Location* gen_stack_var(size_t varSize, size_t stackOffset) {
    Location* reg = gen_reg_var(PTRSIZE, REG_RBP.id);
    Location* off = gen_imm_var(PTRSIZE, varSize + stackOffset);
    
    Location* ea = fastalloc(sizeof(Location));
    *ea = LocEA(varSize, reg, off, NEGATIVE, 1);

    return gen_mem_var(varSize, ea);
}

static Location* start_scratch_reg(size_t size, FILE* out);
static void end_scratch_reg(Location* scratch, FILE* out);

static Location* start_as_dest_reg(Location* other, FILE* out);
static void end_as_dest_reg(Location* as_reg, Location* old, FILE* out);

static Location* start_as_primitive(Location* other, FILE* out);
static void end_as_primitive(Location* as_prim, Location* old, FILE* out);

static Location* start_as_size(size_t size, Location* loc, FILE* out);
static void end_as_size(Location* as_size, Location* old, FILE* out);

static void emiti_move(Location* src, Location* dest, bool sign_ext, FILE* out);

static void emiti_lea(Location* src, Location* dest, FILE* out) {
    Location* dest_as_reg = start_as_dest_reg(dest, out);

    assert(src->type == LOC_EA);

    fputs("lea ", out);
    emit(dest_as_reg, out);
    fputs(", [", out);
    emitEa(src, out);
    fputs("]\n", out);

    end_as_dest_reg(dest_as_reg, dest, out);
} 

static void emiti_enter(FILE* out) {
    fputs("push rbp\n", out);
    fputs("mov rbp, rsp\n", out);
} 

static void emiti_leave(FILE* out) {
    fputs("pop rbp\n", out);
}

static void emiti_jump_cond(Location* target, const char *cc, FILE* out) {
    Location* as_prim = start_as_primitive(target, out);

    fprintf(out, "j%s ", cc);
    emit(as_prim, out);
    fputc('\n', out);

    end_as_primitive(as_prim, target, out);
}

static void emiti_jump(Location* target, FILE* out) {
    emiti_jump_cond(target, "mp" /* jmp */, out);
}

static void emiti_call(Location* target, FILE* out) {
    Location* as_prim = start_as_primitive(target, out);

    fputs("call ", out);
    emit(as_prim, out);
    fputc('\n', out);

    end_as_primitive(as_prim, target, out);
}

static void emiti_storecond(Location* dest, const char *cc, FILE* out) {
    if (dest->bytesWidth == 1) {
        fprintf(out, "set%s ", cc);
        emit(dest, out);
        fputc('\n', out);
    }
    else {
        Location* scratch = start_scratch_reg(1, out);
        emiti_storecond(scratch, cc, out);
        emiti_move(scratch, dest, /* sign extend */ false, out);
        end_scratch_reg(scratch, out);
    }
}

static void emiti_zero(Location* dest, FILE* out) {
    if (dest->type == LOC_REG) {
        if (IsIntReg(dest->v.reg.id)) {
            if (dest->bytesWidth == 8) {
                dest = loc_opt_copy(dest);
                dest->bytesWidth = 4;
            }

            fputs("xor ", out);
            emit(dest, out);
            fputs(", ", out);
            emit(dest, out);
            fputc('\n', out);
        } else {
            fputs("pxor ", out);
            emit(dest, out);
            fputs(", ", out);
            emit(dest, out);
            fputc('\n', out);
        }
        return;
    }

    fputs("mov ", out);
    emit(dest, out);
    fputs(", 0\n", out);
}

static bool equal(Location* a, Location* b) {
    if (a == NULL || b == NULL)
        return false;

    if (a == b)
        return true;

    if (a->type != b->type)
        return false;

    if (a->type == LOC_REG && a->v.reg.id == b->v.reg.id)
        return true;

    if (a->type == LOC_MEM && equal(a->v.mem.address, b->v.mem.address))
        return true;

    if (a->type == LOC_IMM && a->v.imm.bits == b->v.imm.bits)
        return true;

    if (a->type == LOC_EA &&
            equal(a->v.ea.base, b->v.ea.base) &&
            equal(a->v.ea.offset, b->v.ea.offset) && 
            a->v.ea.offsetMul == b->v.ea.offsetMul &&
            a->v.ea.offsetSign == b->v.ea.offsetSign)
        return true;

    return false;
}

static void emiti_move(Location* src, Location *dest, bool sign_ext, FILE* out) {
    if (src->type == LOC_IMM && src->v.imm.bits == 0) {
        emiti_zero(dest, out);
        return;
    }

    if (equal(src, dest)) return;

    if (src->type == LOC_EA) {
        emiti_lea(src, dest, out);
        return;
    }

    if (dest->type == LOC_MEM) {
        if (src->type == LOC_MEM) {
            Location* tmp = start_scratch_reg(src->bytesWidth, out);
            emiti_move(src, tmp, sign_ext, out);
            emiti_move(tmp, dest, sign_ext, out);
            end_scratch_reg(tmp, out);
            return;
        }
    }

    if (src->bytesWidth > dest->bytesWidth) {
        Location* new = loc_opt_copy(src);
        new->bytesWidth = dest->bytesWidth;
        emiti_move(new, dest, sign_ext, out);
        return;
    }

    const char *prefix = "mov ";
    if ((src->type == LOC_MEM && dest->type == LOC_REG && !IsIntReg(dest->v.reg.id)) ||
        (dest->type == LOC_MEM && src->type == LOC_REG && !IsIntReg(src->v.reg.id)))
    {
        if (src->bytesWidth == 4) {
            prefix = "movss ";
        } else if (src->bytesWidth == 8) {
            prefix = "movsd ";
        } else {
            assert(false /* TODO */);
        }
    }
    else if (src->bytesWidth != dest->bytesWidth) {
        if (sign_ext) {
            prefix = "movsx ";
        } else {
            if (dest->bytesWidth == 8 && src->bytesWidth == 4) {
                dest = loc_opt_copy(dest);
                dest->bytesWidth = 4;
            } else {
                prefix = "movzx ";
            }
        }
    }

    fputs(prefix, out);
    emit(dest, out);
    fputs(", ", out);
    emit(src, out);
    fputc('\n', out);
}

static void emiti_cmove(Location* src, Location* dest, const char *cc, FILE* out) {
    assert(src->bytesWidth == dest->bytesWidth);

    fprintf(out, "cmov%s ", cc);
    emit(dest, out);
    fputs(", ", out);
    emit(src, out);
    fputc('\n', out);
}

static void emiti_cmp(Location* a, Location* b, FILE* out);

static void emiti_cmp0(Location* val, FILE* out) {
    if (val->type == LOC_REG) {
        fputs("test ", out); 
        emit(val, out);
        fputs(", ", out);
        emit(val, out);
        fputc('\n', out);
    }
    else if (val->type == LOC_IMM || val->type == LOC_EA) {
        Location* scratch = start_scratch_reg(val->bytesWidth, out);
        emiti_move(val, scratch, false, out);
        emiti_cmp0(scratch, out);
        end_scratch_reg(scratch, out);
    }
    else if (val->type == LOC_MEM) {
        fputs("cmp ", out);
        emit(val, out);
        fputs(", 0\n", out);
    }
    else {
        assert(false);
    }
}

static void emiti_cmp(Location* a, Location* b, FILE* out) {
    if (b->bytesWidth != a->bytesWidth) {
        Location* as_size = start_as_size(a->bytesWidth, b, out);
        emiti_cmp(a, as_size, out);
        end_as_size(as_size, b, out);
        return;
    }

    if (b->type == LOC_IMM && b->v.imm.bits == 0) {
        emiti_cmp0(a, out);
        return;
    }

    if (a->type == LOC_IMM || a->type == LOC_EA) {
        Location* scratch = start_scratch_reg(a->bytesWidth, out);
        emiti_move(a, scratch, false, out);
        emiti_cmp(scratch, b, out);
        end_scratch_reg(scratch, out);
        return;
    }
    
    Location* as_prim_b = start_as_primitive(b, out);

    fputs("cmp ", out);
    emit(a, out);
    fputs(", ", out);
    emit(as_prim_b, out);
    fputc('\n', out);

    end_as_primitive(as_prim_b, b, out);
}

static void emiti_binary(Location* a, Location* b, Location* o, const char * binary, FILE* out) {
    size_t size = o->bytesWidth;

    if (a->type == LOC_MEM && b->type == LOC_MEM) {
        Location* reg_b = start_as_primitive(b, out);
        emiti_binary(a, reg_b, o, binary, out);
        end_as_primitive(reg_b, b, out);
        return;
    }

    if (a->bytesWidth != size) {
        Location* as_size = start_as_size(size, a, out);
        emiti_binary(as_size, b, o, binary, out);
        end_as_size(as_size, a, out);
        return;
    }

    if (b->bytesWidth != size) {
        Location* as_size = start_as_size(size, b, out);
        emiti_binary(a, as_size, o, binary, out);
        end_as_size(as_size, b, out);
        return;
    }

    if (a != o) {
        if (b == o) {
            Location* scratch = start_scratch_reg(size, out);
            emiti_move(a, scratch, false, out);
            emiti_binary(scratch, b, scratch, binary, out);
            emiti_move(scratch, o, false, out);
            end_scratch_reg(scratch, out);
        } else {
            emiti_move(a, o, false, out);
            emiti_binary(o, b, o, binary, out);
        }
        return;
    }

    fprintf(out, "%s ", binary);
    emit(a, out);
    fputs(", ", out);
    emit(b, out);
    fputc('\n', out);
}

static void emiti_unary(Location* v, Location* o, const char * unary, FILE* out) {
    if (v != o) {
        if (o->type == LOC_MEM) {
            Location* scratch = start_scratch_reg(v->bytesWidth, out);
            emiti_move(v, scratch, false, out);
            emiti_unary(scratch, scratch, unary, out);
            emiti_move(scratch, o, false, out);
            end_scratch_reg(scratch, out);
        }
        else {
            emiti_move(v, o, false, out);
            emiti_unary(o, o, unary, out);
        }

        return;
    }

    fprintf(out, "%s ", unary);
    emit(v, out);
    fputc('\n', out);
}

#define SCRATCH_REG (REG_RAX.id)

typedef struct {
    vx_IrType* type;
    Location* location;
    size_t heat; // if heat == 2 then one producer and one consumer
} VarData;

VarData* varData = NULL;

static Location* as_loc(size_t width, vx_IrValue val) {
    switch (val.type) {
    case VX_IR_VAL_VAR:
        return varData[val.var].location;

    case VX_IR_VAL_IMM_INT:
        return gen_imm_var(width, val.imm_int);

    case VX_IR_VAL_IMM_FLT: {
        int64_t bits = *(int64_t*)&val.imm_flt;
        return gen_imm_var(width, bits);
    }

    case VX_IR_VAL_ID: {
        Location* loc = fastalloc(sizeof(Location));
        static char str[16];
        sprintf(str, ".l%zu", val.id);
        *loc = LocLabel(faststrdup(str));
        return loc;
    }

    case VX_IR_VAL_BLOCKREF: {
        Location* loc = fastalloc(sizeof(Location));
        *loc = LocLabel(val.block->name);
        return loc;
    }

    default:
        assert(false);
        return NULL;
    }
}

static void emit_call_arg_load(vx_IrOp* callOp, FILE* file) {
    assert(callOp->args_len <= 6);

    vx_IrValue fn = *vx_IrOp_param(callOp, VX_IR_NAME_ADDR);

    vx_IrTypeRef type = vx_IrValue_type(callOp->parent, fn);
    assert(type.ptr);
    assert(type.ptr->kind == VX_IR_TYPE_FUNC);
    vx_IrTypeFunc fnType = type.ptr->func;
    vx_IrTypeRef_drop(type);

    char regs[6] = { REG_RDI.id, REG_RSI.id, REG_RDX.id, REG_RCX.id, REG_R8.id, REG_R9.id };

    for (size_t i = 0; i < callOp->args_len; i ++) {
        vx_IrType* type = fnType.args[i];
        size_t size = vx_IrType_size(type);
        Location* src = as_loc(size, callOp->args[i]);
        Location* dest = gen_reg_var(size, regs[i]);
        emiti_move(src, dest, false, file);
    }
}

static void emit_call_ret_store(vx_IrOp* callOp, FILE* file) {
    if (callOp->outs_len > 0) {
        vx_IrVar ret = callOp->outs[0].var;
        Location* retl = varData[ret].location;
        Location* eax = gen_reg_var(retl->bytesWidth, REG_RAX.id);
        emiti_move(eax, retl, false, file);
    }
}

static void emit_condmove(vx_IrOp* op, const char *cc, FILE* file) {
    vx_IrValue vthen = *vx_IrOp_param(op, VX_IR_NAME_COND_THEN);
    assert(vx_IrOp_param(op, VX_IR_NAME_COND_ELSE) == NULL);
    Location* out = varData[op->outs[0].var].location;

    emiti_cmove(as_loc(out->bytesWidth, vthen), out, cc, file);
}

static size_t var_used_after_and_before_overwrite(vx_IrOp* after, vx_IrVar var) {
    size_t i = 0;
    for (vx_IrOp *op = after->next; op; op = op->next) {
        for (size_t o = 0; o < op->outs_len; o ++)
            if (op->outs[o].var == var)
                break;

        if (vx_IrOp_var_used(op, var))
            i ++;
    }
    return i;
}

static bool ops_after_and_before_usage_or_overwrite(vx_IrOp* after, vx_IrVar var) {
    return after->next && !vx_IrOp_var_used(after->next, var);
}

static void emiti_flt_to_int(Location* src, Location* dest, FILE* file) {
    Location* dest_v = start_as_dest_reg(dest, file);
    if (dest_v->bytesWidth < 4)
        dest_v->bytesWidth = 4;

    const char *op;
    if (src->bytesWidth == 4) {
        op = "cvttss2si ";
    } else if (src->bytesWidth == 8) {
        op = "cvttsd2si ";
    } else {
        assert(false);
    }

    fputs(op, file);
    emit(dest_v, file);
    fputs(", ", file);
    emit(src, file);
    fputc('\n', file);

    end_as_dest_reg(dest_v, dest, file);
}

static void emiti_int_to_flt(Location* src, Location* dest, FILE* file) {
    Location* dest_v = start_as_dest_reg(dest, file);
    Location* src_v = start_as_size(dest->bytesWidth, src, file);
    
    const char *op;
    if (dest->bytesWidth == 4) {
        op = "cvtsi2ss ";
    } else if (dest->bytesWidth == 8) {
        op = "cvtsi2sd ";
    } else {
        assert(false);
    }

    fputs(op, file);
    emit(dest_v, file);
    fputs(", ", file);
    emit(src_v, file);
    fputc('\n', file);

    end_as_size(src_v, src, file);
    end_as_dest_reg(dest_v, dest, file);
}

static void emiti_bittest(Location* val, Location* idx, FILE* file) {
    if (val->bytesWidth == 1) {
        Location* valv = start_as_size(2, val, file);
        emiti_bittest(valv, idx, file);
        end_as_size(valv, val, file);
        return;
    }

    Location* idxv = start_as_primitive(idx, file);

    fputs("bt ", file);
    emit(val, file);
    fputs(", ", file);
    emit(idx, file);
    fputc('\n', file);

    end_as_primitive(idxv, idx, file);
}

static vx_IrOp* emiti(vx_IrBlock* block, vx_IrOp *prev, vx_IrOp* op, FILE* file) {
    switch (op->id) {
        case VX_IR_OP_FROMFLT: // "val" 
            {
                vx_IrValue val = *vx_IrOp_param(op, VX_IR_NAME_VALUE);
                vx_IrVar out = op->outs[0].var;
                Location* outLoc = varData[out].location;
                emiti_flt_to_int(as_loc(outLoc->bytesWidth, val), outLoc, file);
            } break;

        case VX_IR_OP_TOFLT: // "val" 
            {
                vx_IrValue val = *vx_IrOp_param(op, VX_IR_NAME_VALUE);
                vx_IrVar out = op->outs[0].var;
                Location* outLoc = varData[out].location;
                emiti_int_to_flt(as_loc(outLoc->bytesWidth, val), outLoc, file);
            } break;
        
        case VX_IR_OP_REINTERPRET: // "val"
        case VX_IR_OP_BITCAST:     // "val"
        case VX_IR_OP_IMM: // "val"
        case VX_IR_OP_ZEROEXT:     // "val"
            {
                vx_IrValue val = *vx_IrOp_param(op, VX_IR_NAME_VALUE);
                vx_IrVar out = op->outs[0].var;
                Location* outLoc = varData[out].location;
                emiti_move(as_loc(outLoc->bytesWidth, val), outLoc, false, file);
            } break;

        case VX_IR_OP_SIGNEXT:     // "val"
            {
                vx_IrValue val = *vx_IrOp_param(op, VX_IR_NAME_VALUE);
                vx_IrVar out = op->outs[0].var;
                Location* outLoc = varData[out].location;
                emiti_move(as_loc(outLoc->bytesWidth, val), outLoc, true, file);
            } break;

        case VX_IR_OP_LOAD:            // "addr"
        case VX_IR_OP_LOAD_VOLATILE:   // "addr"
            {
                vx_IrValue val = *vx_IrOp_param(op, VX_IR_NAME_VALUE);
                vx_IrVar out = op->outs[0].var;
                Location* outLoc = varData[out].location;
                Location* mem = gen_mem_var(outLoc->bytesWidth, as_loc(PTRSIZE, val));
                emiti_move(mem, outLoc, false, file);
            } break;

        case VX_IR_OP_STORE:           // "addr", "val"
        case VX_IR_OP_STORE_VOLATILE:  // "addr", "val"
            {
                vx_IrValue addrV = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
                vx_IrValue valV = *vx_IrOp_param(op, VX_IR_NAME_VALUE);
                vx_IrTypeRef type = vx_IrValue_type(block, valV);
                assert(type.ptr);

                Location* addr = as_loc(PTRSIZE, addrV);
                Location* val = as_loc(vx_IrType_size(type.ptr), valV);
                Location* mem = gen_mem_var(val->bytesWidth, addr);
                emiti_move(val, mem, false, file);

                vx_IrTypeRef_drop(type);
            } break;

        case VX_IR_OP_PLACE:           // "var"
            {
                vx_IrValue valV = *vx_IrOp_param(op, VX_IR_NAME_VALUE);
                // TODO: stop imm inliner from inlining val and make it move in var if in var in input
                assert(valV.type == VX_IR_VAL_VAR);

                Location* loc = varData[valV.var].location;
                assert(loc->type == LOC_MEM);

                vx_IrVar out = op->outs[0].var;
                Location* outLoc = varData[out].location;

                emiti_move(loc->v.mem.address, outLoc, false, file);
            } break;

        case VX_IR_OP_ADD: // "a", "b"
        case VX_IR_OP_SUB: // "a", "b"
            {
                Location* o = varData[op->outs[0].var].location;
                assert(o);
                Location* a = as_loc(o->bytesWidth, *vx_IrOp_param(op, VX_IR_NAME_OPERAND_A));
                Location* b = as_loc(o->bytesWidth, *vx_IrOp_param(op, VX_IR_NAME_OPERAND_B));

                if (!equal(o, a) && a->type == LOC_REG) {
                    int sign = op->id == VX_IR_OP_ADD ? 1 : -1;

                    Location* ea = fastalloc(sizeof(Location));

                    Location* as = start_as_size(PTRSIZE, a, file);
                    Location* bs = start_as_size(PTRSIZE, b, file);
                    *ea = LocEA(o->bytesWidth, a, b, sign, 1);
                    emiti_move(ea, o, false, file);
                    end_as_size(bs, b, file);
                    end_as_size(as, a, file);

                    break;
                }
            } // no break 
        
        case VX_IR_OP_MOD: // "a", "b"
        case VX_IR_OP_MUL: // "a", "b"
        case VX_IR_OP_UDIV: // "a", "b"
        case VX_IR_OP_SDIV: // "a", "b"
        case VX_IR_OP_AND: // "a", "b"
        case VX_IR_OP_BITWISE_AND: // "a", "b"
        case VX_IR_OP_OR:  // "a", "b"
        case VX_IR_OP_BITIWSE_OR:  // "a", "b"
        case VX_IR_OP_SHL: // "a", "b"
        case VX_IR_OP_SHR: // "a", "b"
            {
                Location* o = varData[op->outs[0].var].location;
                assert(o);
                Location* a = as_loc(o->bytesWidth, *vx_IrOp_param(op, VX_IR_NAME_OPERAND_A));
                Location* b = as_loc(o->bytesWidth, *vx_IrOp_param(op, VX_IR_NAME_OPERAND_B));

                const char * bin;
                switch (op->id) {
                case VX_IR_OP_ADD: bin = "add"; break;
                case VX_IR_OP_SUB: bin = "sub"; break;
                case VX_IR_OP_MOD: bin = "mod"; break;
                case VX_IR_OP_MUL: bin = "imul"; break;
                case VX_IR_OP_UDIV: bin = "div"; break;
                case VX_IR_OP_SDIV: bin = "idiv"; break;
                case VX_IR_OP_AND: 
                case VX_IR_OP_BITWISE_AND: bin = "and"; break;
                case VX_IR_OP_OR:
                case VX_IR_OP_BITIWSE_OR: bin = "orr"; break;
                case VX_IR_OP_SHL: bin = "shl"; break;
                case VX_IR_OP_SHR: bin = "shr"; break;

                default: assert(false); break;
                }

                emiti_binary(a, b, o, bin, file);
            } break;

        case VX_IR_OP_NOT: // "val"
        case VX_IR_OP_BITWISE_NOT: // "val"
            {
                Location* o = varData[op->outs[0].var].location;
                assert(o);
                Location* v = as_loc(o->bytesWidth, *vx_IrOp_param(op, VX_IR_NAME_VALUE));
                emiti_unary(v, o, "not", file);
            } break;

        case VX_IR_OP_UGT:  // "a", "b"
        case VX_IR_OP_UGTE: // "a", "b"
        case VX_IR_OP_ULT:  // "a", "b"
        case VX_IR_OP_ULTE: // "a", "b"
        case VX_IR_OP_SGT:  // "a", "b"
        case VX_IR_OP_SGTE: // "a", "b"
        case VX_IR_OP_SLT:  // "a", "b"
        case VX_IR_OP_SLTE: // "a", "b"
        case VX_IR_OP_EQ:  // "a", "b"
        case VX_IR_OP_NEQ: // "a", "b"
        case VX_IR_OP_BITEXTRACT: // "idx", "val"
            {
                vx_IrVar ov = op->outs[0].var;
                Location* o = varData[ov].location;
                assert(o);

                const char *cc;

                if (op->id == VX_IR_OP_BITEXTRACT) {
                    Location* idx = as_loc(1, *vx_IrOp_param(op, VX_IR_NAME_IDX));
                    Location* val = as_loc(PTRSIZE, *vx_IrOp_param(op, VX_IR_NAME_VALUE));

                    emiti_bittest(val, idx, file);

                    cc = "nz";
                }
                else {
                    Location* a = as_loc(PTRSIZE, *vx_IrOp_param(op, VX_IR_NAME_OPERAND_A));
                    Location* b = as_loc(PTRSIZE, *vx_IrOp_param(op, VX_IR_NAME_OPERAND_B));

                    emiti_cmp(a, b, file);

                    switch (op->id) {
                    case VX_IR_OP_UGT: cc = "a"; break;
                    case VX_IR_OP_ULT: cc = "b"; break;
                    case VX_IR_OP_UGTE: cc = "ae"; break;
                    case VX_IR_OP_ULTE: cc = "be"; break;
                    case VX_IR_OP_SGT: cc = "g"; break;
                    case VX_IR_OP_SLT: cc = "l"; break;
                    case VX_IR_OP_SGTE: cc = "ge"; break;
                    case VX_IR_OP_SLTE: cc = "le"; break;
                    case VX_IR_OP_EQ: cc = "e"; break;
                    case VX_IR_OP_NEQ: cc = "ne"; break;

                    default: assert(false); break;
                    }
                }

                if (!(op->next && vx_IrOp_var_used(op->next, ov) && (op->next->id == VX_IR_OP_CMOV || op->next->id == VX_IR_OP_CONDTAILCALL || op->next->id == VX_LIR_OP_COND))) {
                    emiti_storecond(o, cc, file);
                }

                if (op->next) {
                    vx_IrOpType ty = op->next->id;
                    if (ty == VX_IR_OP_CMOV) {
                        vx_IrValue v = *vx_IrOp_param(op->next, VX_IR_NAME_COND);
                        if (v.type == VX_IR_VAL_VAR && v.var == ov) {
                            emit_condmove(op->next, cc, file);
                            return op->next->next;
                        }
                    }

                    if (ty == VX_LIR_OP_COND) {
                        vx_IrValue id = *vx_IrOp_param(op->next, VX_IR_NAME_ID);
                        vx_IrValue v = *vx_IrOp_param(op->next, VX_IR_NAME_COND);
                        if (v.type == VX_IR_VAL_VAR && v.var == ov) {
                            emiti_jump_cond(as_loc(PTRSIZE, id), cc, file);
                            return op->next->next;
                        }
                    }

                    if (ty == VX_IR_OP_CONDTAILCALL) {
                        assert(!needEpilog);
                        vx_IrValue addr = *vx_IrOp_param(op->next, VX_IR_NAME_ADDR);
                        vx_IrValue v = *vx_IrOp_param(op->next, VX_IR_NAME_COND);
                        if (v.type == VX_IR_VAL_VAR && v.var == ov) {
                            emit_call_arg_load(op->next, file);
                            emiti_jump_cond(as_loc(PTRSIZE, addr), cc, file);
                            emit_call_ret_store(op->next, file);
                            return op->next->next;
                        }
                    }
                }
            } break;


        case VX_LIR_OP_COND:         // "id", "cond": bool
            {
                vx_IrValue id = *vx_IrOp_param(op, VX_IR_NAME_ID);
                vx_IrValue cond = *vx_IrOp_param(op, VX_IR_NAME_COND);
                Location* loc = as_loc(1, cond);
                emiti_cmp0(loc, file);
                emiti_jump_cond(as_loc(PTRSIZE, id), "nz", file);
            } break;

        case VX_IR_OP_CONDTAILCALL:  // "addr": int / fnref, "cond": bool
            {
                assert(!needEpilog);
                vx_IrValue addr = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
                vx_IrValue cond = *vx_IrOp_param(op, VX_IR_NAME_COND);
                Location* loc = as_loc(1, cond);
                emiti_cmp0(loc, file);
                emit_call_arg_load(op, file);
                emiti_jump_cond(as_loc(PTRSIZE, addr), "nz", file);
            } break;

        case VX_IR_OP_CMOV:          // "cond": ()->bool, "then": value, "else": value
            {
                vx_IrValue cond = *vx_IrOp_param(op, VX_IR_NAME_COND);
                Location* loc = as_loc(1, cond);
                emiti_cmp0(loc, file);
                emit_condmove(op, "nz", file);
            } break;

        case VX_LIR_OP_LABEL:        // "id"
            {
                vx_IrValue id = *vx_IrOp_param(op, VX_IR_NAME_ID);
                fprintf(file, ".l%zu:\n", id.id);
            } break;

        case VX_LIR_OP_GOTO:         // "id"
            {
                Location* tg = as_loc(PTRSIZE, *vx_IrOp_param(op, VX_IR_NAME_ID));
                emiti_jump(tg, file);
            } break;

        case VX_IR_OP_CALL:          // "addr": int / fnref
            {
                vx_IrValue addr = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
                emit_call_arg_load(op, file);
                emiti_call(as_loc(PTRSIZE, addr), file);
                emit_call_ret_store(op, file);
            } break;

        case VX_IR_OP_TAILCALL:      // "addr": int / fnref
            {
                assert(!needEpilog);
                vx_IrValue addr = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
                emit_call_arg_load(op, file);
                emiti_jump(as_loc(PTRSIZE, addr), file);
            } break;

        default:
            assert(false);
            break;
    }

    return op->next;
}

vx_cg_x86stupid vx_cg_x86stupid_options = (vx_cg_x86stupid) {
    .use_red_zone = true,
};

void vx_cg_x86stupid_gen(vx_IrBlock* block, FILE* out) {
    fprintf(out, "%s:\n", block->name);

    assert(block->is_root);

    bool is_leaf = vx_IrBlock_ll_isleaf(block);
    bool use_rax = is_leaf &&
                   block->outs_len > 0 &&
                   !block->as_root.vars[block->outs[0]].ever_placed;
    vx_IrType* use_rax_type = NULL;
    if (use_rax) {
        use_rax_type = vx_IrBlock_typeof_var(block, block->outs[0]);
        assert(use_rax_type);
        if (vx_IrType_size(use_rax_type) > 8) {
            use_rax = false;
            use_rax_type = NULL;
        }
    }

    bool anyPlaced = false;
    for (vx_IrVar var = 0; var < block->as_root.vars_len; var ++) {
        if (block->as_root.vars[var].ever_placed) {
            anyPlaced = true;
        }
    }

    // arguments 1-6 : RDI, RSI, RDX, RCX, R8, R9
    // always used   : RBP, RSP, R10 
    // calle cleanup : RBX, R12, R13, R14, R15

    size_t availableRegistersCount = 2;
    if (is_leaf && !use_rax) {
        availableRegistersCount = 3;
    }
    char * availableRegisters = fastalloc(availableRegistersCount);
    availableRegisters[0] = REG_R10.id;
    availableRegisters[1] = REG_R11.id;
    if (is_leaf && !use_rax) {
        availableRegisters[2] = REG_RAX.id;
    }

    size_t anyIntArgsCount = block->ins_len;
    size_t anyCalledIntArgsCount = 0;
    size_t anyCalledXmmArgsCount = 0;
    // max arg len 
    for (vx_IrOp* op = block->first; op != NULL; op = op->next) {
        if (op->id == VX_IR_OP_CALL || op->id == VX_IR_OP_TAILCALL || op->id == VX_IR_OP_CONDTAILCALL) {
            vx_IrValue addr = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
            vx_IrTypeRef ty = vx_IrValue_type(block, addr);
            assert(ty.ptr != NULL);
            assert(ty.ptr->kind == VX_IR_TYPE_FUNC);

            vx_IrTypeFunc fn = ty.ptr->func;

            size_t usedInt = 0;
            size_t usedXmm = 0;

            for (size_t i = 0; i < fn.args_len; i ++) {
                vx_IrType* arg = fn.args[i];
                assert(arg->kind == VX_IR_TYPE_KIND_BASE);
                
                if (arg->base.isfloat)
                    usedXmm ++;
                else usedInt ++;
            }

            if (usedInt > anyCalledIntArgsCount)
                anyCalledIntArgsCount = usedInt;
            if (usedXmm > anyCalledXmmArgsCount)
                anyCalledXmmArgsCount = usedXmm;

            vx_IrTypeRef_drop(ty);
        }
    }
    if (anyCalledIntArgsCount > anyIntArgsCount)
        anyIntArgsCount = anyCalledIntArgsCount;

    if (anyIntArgsCount < 6) {
        size_t extraAv = 6 - anyIntArgsCount;
        availableRegisters = fastrealloc(availableRegisters, availableRegistersCount, availableRegistersCount + extraAv);

        availableRegisters[availableRegistersCount] = REG_R9.id;
        if (extraAv > 1) {
            availableRegisters[availableRegistersCount + 1] = REG_R8.id;
            if (extraAv > 2) {
                availableRegisters[availableRegistersCount + 2] = REG_RCX.id;
                if (extraAv > 3) {
                    availableRegisters[availableRegistersCount + 3] = REG_RDX.id;
                    if (extraAv > 4) {
                        availableRegisters[availableRegistersCount + 4] = REG_RSI.id;
                        if (extraAv > 5) {
                            availableRegisters[availableRegistersCount + 5] = REG_RDI.id;
                        }
                    }
                }
            }
        }

        availableRegistersCount += extraAv;
    }

    varData = calloc(block->as_root.vars_len, sizeof(VarData));
    for (size_t i = 0; i < block->ins_len; i ++) {
        varData[block->ins[i].var].type = block->ins[i].type;
    }
    for (vx_IrOp *op = block->first; op != NULL; op = op->next) {
        for (size_t i = 0; i < op->outs_len; i ++) {
            vx_IrVar v = op->outs[i].var;
            varData[v].type = op->outs[i].type;
            varData[v].heat ++;
        }
        for (size_t i = 0; i < op->args_len; i ++) {
            vx_IrValue val = op->args[i];
            if (val.type == VX_IR_VAL_VAR) {
                varData[val.var].heat ++;
            }
        }
        for (size_t i = 0; i < op->params_len; i ++) {
            vx_IrValue val = op->params[i].val;
            if (val.type == VX_IR_VAL_VAR) {
                varData[val.var].heat ++;
            }
        }
    }

    size_t stackOff = 0;

    /* ======================== VAR ALLOC ===================== */ 
    {
        signed long long highestHeat = 0;
        for (vx_IrVar var = 0; var < block->as_root.vars_len; var ++) {
            size_t heat = varData[var].heat;
            if (heat > highestHeat) {
                highestHeat = heat;
            }
        }

        vx_IrVar* varsHotFirst = calloc(block->as_root.vars_len, sizeof(vx_IrVar));
        size_t varsHotFirstLen = 0;

        char* varsSorted = calloc(block->as_root.vars_len, sizeof(char));
        if (use_rax) {
            varsSorted[block->outs[0]] = true;
        }
        for (size_t i = anyCalledIntArgsCount; i < block->ins_len; i ++) {
            vx_IrVar var = block->ins[i].var;
            varsSorted[var] = true;
        }
        for (; highestHeat >= 0; highestHeat --) {
            for (vx_IrVar var = 0; var < block->as_root.vars_len; var ++) {
                if (varsSorted[var]) continue;

                if (varData[var].heat == highestHeat) {
                    varsHotFirst[varsHotFirstLen ++] = var;
                    varsSorted[var] = true;
                }
            }
        }
        free(varsSorted);

        size_t varId = 0;
        for (size_t i = 0; i < availableRegistersCount; i ++) {
            char reg = availableRegisters[i];

            if (varId >= varsHotFirstLen) break;

            for (; varId < varsHotFirstLen; varId ++) {
                vx_IrVar var = varsHotFirst[varId];
                
                if (varData[var].heat == 0) {
                    varData[var].location = NULL;
                    continue;
                }

                vx_IrType* type = varData[var].type;
                if (type == NULL) continue;

                size_t size = vx_IrType_size(type);
                if (size > 8) continue;
                
                if (block->as_root.vars[var].ever_placed) continue; 

                size = widthToWidthWidth(size);
                varData[var].location = gen_reg_var(size, reg);
                break;
            }
            varId ++;
        }

        if (use_rax) {
            size_t size = vx_IrType_size(use_rax_type);
            size = widthToWidthWidth(size);
            varData[block->outs[0]].location = gen_reg_var(size, REG_RAX.id);
        }

        char intArgRegs[6] = { REG_RDI.id, REG_RSI.id, REG_RDX.id, REG_RCX.id, REG_R8.id, REG_R9.id };

        size_t id_i = 0;
        size_t id_f = 0;
        struct VarD {
            vx_IrVar var;
            Location* argLoc;
        };
        struct VarD * toMove = NULL;
        size_t toMoveLen = 0;
        for (size_t i = 0; i < block->ins_len; i ++) {
            vx_IrTypedVar var = block->ins[i];
            assert(var.type->kind == VX_IR_TYPE_KIND_BASE);
            size_t size = vx_IrType_size(var.type);

            if (var.type->base.isfloat) {
                Location* loc;
                if (id_i >= 8) {
                    loc = gen_stack_var(size, stackOff);
                    stackOff += size;
                    anyPlaced = true;
                } else {
                    size = widthToWidthWidth(size);
                    loc = gen_reg_var(size, IntRegCount + id_i);
                }

                if (id_i >= anyCalledXmmArgsCount) {
                    varData[var.var].location = loc;
                } else {
                    toMove = realloc(toMove, sizeof(struct VarD) * (toMoveLen + 1));
                    toMove[toMoveLen ++] = (struct VarD) {
                        .var = var.var,
                        .argLoc = loc
                    };
                }

                id_f ++;
            } else {
                Location* loc;
                if (id_i >= 6) {
                    loc = gen_stack_var(size, stackOff);
                    stackOff += size;
                    anyPlaced = true;
                } else {
                    size = widthToWidthWidth(size);
                    loc = gen_reg_var(size, intArgRegs[id_i]);
                }

                if (id_i >= anyCalledIntArgsCount) {
                    varData[var.var].location = loc;
                } else {
                    toMove = realloc(toMove, sizeof(struct VarD) * (toMoveLen + 1));
                    toMove[toMoveLen ++] = (struct VarD) {
                        .var = var.var,
                        .argLoc = loc
                    };
                } 

                id_i ++;
            }
        }

        for (; varId < varsHotFirstLen; varId ++) {
            if (varData[varId].heat == 0) {
                varData[varId].location = NULL;
                continue;
            }

            vx_IrVar var = varsHotFirst[varId];
            if (varData[var].type == NULL) continue;
            size_t size = vx_IrType_size(varData[var].type);
            varData[var].location = gen_stack_var(size, stackOff);
            stackOff += size;
            anyPlaced = true;
        }

        for (size_t i = 0; i < toMoveLen; i ++) {
            Location* dst = varData[toMove[i].var].location;
            Location* src = toMove[i].argLoc;
            emiti_move(src, dst, false, out);
        }

        free(toMove);
        free(varsHotFirst);
    }

    bool needProlog = (stackOff > 0 && !is_leaf) ||
                      stackOff > 128 ||
                      (stackOff > 0 && !vx_cg_x86stupid_options.use_red_zone);

    needEpilog = false;
    if (anyPlaced || needProlog) {
        emiti_enter(out);
        needEpilog = true;
    }

    vx_IrBlock_ll_finalize(block, needEpilog);

    if (needProlog) {
        if (is_leaf && vx_cg_x86stupid_options.use_red_zone) {
            size_t v = 128 - stackOff;
            fprintf(out, "sub rsp, %zu\n", v);
        } else {
            fprintf(out, "sub rsp, %zu\n", stackOff);
        }
    }

    vx_IrOp* op = block->first;
    vx_IrOp* prev = NULL;

    while (op != NULL) {
        vx_IrOp* new = emiti(block, prev, op, out);
        prev = op;
        op = new;
    }

    if (block->outs_len >= 1) {
        VarData vd = varData[block->outs[0]];
        Location* src = vd.location;
        
        assert(vd.type != NULL);

        char reg = REG_RAX.id;
        if (vd.type->kind == VX_IR_TYPE_KIND_BASE && vd.type->base.isfloat)
            reg = REG_XMM0.id;

        Location* dest = gen_reg_var(src->bytesWidth, reg);
        emiti_move(src, dest, false, out);
    }

    if (block->outs_len >= 2) {
        VarData vd = varData[block->outs[1]];
        Location* src = vd.location;
        
        assert(vd.type != NULL);

        char reg = REG_RDX.id;
        if (vd.type->kind == VX_IR_TYPE_KIND_BASE && vd.type->base.isfloat)
            reg = REG_XMM1.id;

        Location* dest = gen_reg_var(src->bytesWidth, reg);
        emiti_move(src, dest, false, out);
    }

    assert(block->outs_len <= 2);

    if (needEpilog)
        emiti_leave(out);
    fputs("ret\n", out);

    free(varData);
    varData = NULL;
}

static Location* start_scratch_reg(size_t size, FILE* out) {
    if (size > 8) { // TODO 
        if (RegLut[REG_XMM8.id]->stored) {
            assert(RegLut[REG_XMM9.id]->stored == NULL);
            Location* loc = gen_reg_var(size, REG_XMM9.id);
            RegLut[REG_XMM9.id]->stored = loc;
            return loc;
        }

        Location* loc = gen_reg_var(size, REG_XMM8.id);
        RegLut[REG_XMM8.id]->stored = loc;
        return loc;
    }

    assert(RegLut[SCRATCH_REG]->stored == NULL); // TODO 
    Location* loc = gen_reg_var(size, SCRATCH_REG);
    RegLut[SCRATCH_REG]->stored = loc;
    return loc;
}

static void end_scratch_reg(Location* loc, FILE* out) {
    assert(loc->type == LOC_REG);

    char regId = loc->v.reg.id;
    assert(RegLut[regId]->stored == loc);
    RegLut[regId]->stored = NULL;

    loc->type = LOC_INVALID;
}

static Location* start_as_dest_reg(Location* other, FILE* out) {
    if (other->type == LOC_MEM) {
        return start_scratch_reg(other->bytesWidth, out);
    } else {
        assert(other->type == LOC_REG);
    }

    return other;
}

static void end_as_dest_reg(Location* as_reg, Location* old, FILE* out) {
    if (old->type == LOC_MEM) {
        emiti_move(as_reg, old, false, out);
        end_scratch_reg(as_reg, out);
    }
}

static Location* start_as_primitive(Location* other, FILE* out) {
    assert(other->type != LOC_INVALID);

    if (other->type == LOC_EA) {
        Location* s = start_scratch_reg(other->bytesWidth, out);
        emiti_lea(other, s, out);
        return s;
    }

    if (other->type == LOC_MEM) {
        Location* s = start_scratch_reg(other->bytesWidth, out);
        emiti_move(other, s, false, out);
        return s;
    }

    return other;
}

static void end_as_primitive(Location* as_prim, Location* old, FILE* out) {
    if (old->type == LOC_EA || old->type == LOC_MEM) {
        end_scratch_reg(as_prim, out);
    }
}

static Location* start_as_size(size_t size, Location* loc, FILE* out) {
    if (loc->bytesWidth == size) {
        return loc;
    }

    if ((loc->type == LOC_REG && loc->bytesWidth > size) || loc->type == LOC_MEM || loc->type == LOC_IMM || loc->type == LOC_EA) {
        Location* copy = loc_opt_copy(loc);
        copy->bytesWidth = size;
        return copy;
    }

    Location* prim = start_scratch_reg(size, out);
    emiti_move(prim, loc, false, out);

    return prim;
}

static void end_as_size(Location* as_size, Location* old, FILE* out) {
    if (old->bytesWidth == as_size->bytesWidth) return;
    if ((old ->type == LOC_REG && old->bytesWidth > as_size->bytesWidth) || old->type == LOC_MEM || old->type == LOC_IMM || old->type == LOC_EA) return;

    assert(as_size->type == LOC_REG);
    end_scratch_reg(as_size, out);
}
