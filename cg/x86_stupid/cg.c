#include "cg.h"
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

struct Location;

typedef struct {
    size_t id;
    const char * name[4];

    struct Location * stored;
} RegType;

#define MkRegType(idi, name0, name1, name2, name3) (RegType) { .id = (idi), .name = { name0, name1, name2, name3 }, .stored = NULL }

static RegType REG_RAX = MkRegType(0,  "al",   "ax",   "eax",  "rax");
static RegType REG_RBX = MkRegType(1,  "bl",   "bx",   "ebx",  "rbx");
static RegType REG_RCX = MkRegType(2,  "cl",   "cx",   "ecx",  "rcx");
static RegType REG_RSP = MkRegType(3,  "spl",  "sp",   "esp",  "rsp");
static RegType REG_RBP = MkRegType(4,  "bpl",  "bp",   "ebp",  "rbp");
static RegType REG_RDX = MkRegType(5,  "dl",   "dx",   "edx",  "rdx");
static RegType REG_RSI = MkRegType(6,  "sil",  "si",   "esi",  "rsi");
static RegType REG_RDI = MkRegType(7,  "dil",  "di",   "edi",  "rdi");
static RegType REG_R8  = MkRegType(8,  "r8b",  "r8w",  "r8d",  "r8" );
static RegType REG_R9  = MkRegType(9,  "r9b",  "r9w",  "r9d",  "r9" );
static RegType REG_R10 = MkRegType(10, "r10b", "r10w", "r10d", "r10");
static RegType REG_R11 = MkRegType(11, "r11b", "r11w", "r11d", "r11");
static RegType REG_R12 = MkRegType(12, "r12b", "r12w", "r12d", "r12");
static RegType REG_R13 = MkRegType(13, "r13b", "r13w", "r13d", "r13");
static RegType REG_R14 = MkRegType(14, "r14b", "r14w", "r14d", "r14");
static RegType REG_R15 = MkRegType(15, "r15b", "r15w", "r15d", "r15");
#define RegCount (16)

static RegType* RegLut[RegCount] = {
    &REG_RAX, &REG_RBX, &REG_RCX, &REG_RSP, &REG_RBP, &REG_RDX, &REG_RSI, &REG_RDI,
    &REG_R8,  &REG_R9,  &REG_R10, &REG_R11, &REG_R12, &REG_R13, &REG_R14, &REG_R15
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

#define LocImm(width, value) ((Location) { .bytesWidth = (width), .type = LOC_IMM, .v.imm = (value) })
#define LocReg(width, regId) ((Location) { .bytesWidth = (width), .type = LOC_REG, .v.reg = (regId) })
#define LocEA(width, basee, off, sign, mul) ((Location) { .bytesWidth = (width), .type = LOC_EA, .v.ea.base = (basee), .v.ea.offsetSign = (sign), .v.ea.offset = (off), .v.ea.offsetMul = (mul) })
#define LocMem(width, eaa) ((Location) { .bytesWidth = (width), .type = LOC_MEM, .v.mem.address = (eaa) })
#define LocLabel(str) ((Location) { .type = LOC_LABEL, .v.label.label = (str) })

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

        case LOC_INVALID: {
            assert(false);
            return NULL;
        }
    }
}

static char widthToWidthWidth[] = {
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

static char widthToWidthIdx[] = {
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

static const char * widthWidthToASM[] = {
    [0] = "wtf",
    [1] = "byte",
    [2] = "word",
    [4] = "dword",
    [8] = "qword",
};

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
            char ww = widthToWidthWidth[loc->bytesWidth];
            const char * str = widthWidthToASM[ww];
            fputs(str, out);
            fputs(" [", out);
            emit(loc->v.mem.address, out);
            fputs("]", out);
        } break;

        case LOC_REG:
            fputs(RegLut[loc->v.reg.id]->name[widthToWidthIdx[loc->bytesWidth]], out);
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
    Location* reg = gen_reg_var(8, REG_RBP.id);
    Location* off = gen_imm_var(8, varSize + stackOffset);
    
    Location* ea = fastalloc(sizeof(Location));
    *ea = LocEA(8, reg, off, NEGATIVE, 1);

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

static void emiti_move(Location* src, Location *dest, bool sign_ext, FILE* out) {
    if (src == dest) return;

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
    if (src->bytesWidth != dest->bytesWidth) {
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
    
    vx_IrTypeFunc fnType;
    if (fn.type == VX_IR_VAL_VAR) {
        fnType = varData[fn.var].type->func;
    }
    else if (fn.type == VX_IR_VAL_BLOCKREF) {
        fnType = vx_IrBlock_type(fn.block).ptr->func;
    }
    else {
        assert(false);
    }

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

static vx_IrOp* emiti(vx_IrOp *prev, vx_IrOp* op, FILE* file) {
    switch (op->id) {
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
                Location* mem = gen_mem_var(outLoc->bytesWidth, as_loc(8, val));
                emiti_move(mem, outLoc, false, file);
            } break;

        case VX_IR_OP_STORE:           // "addr", "val"
        case VX_IR_OP_STORE_VOLATILE:  // "addr", "val"
            {
                vx_IrValue addrV = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
                vx_IrValue valV = *vx_IrOp_param(op, VX_IR_NAME_VALUE);
                // TODO: stop imm inliner from inlining val and make it move in var if in var in input
                assert(valV.type == VX_IR_VAL_VAR);

                Location* addr = as_loc(8, addrV);
                Location* val = varData[valV.var].location;
                Location* mem = gen_mem_var(val->bytesWidth, addr);
                emiti_move(val, mem, false, file);
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
        case VX_IR_OP_MOD: // "a", "b"
        case VX_IR_OP_MUL: // "a", "b"
        case VX_IR_OP_DIV: // "a", "b"
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
                case VX_IR_OP_MUL: bin = "mul"; break; // TODO: imul?
                case VX_IR_OP_DIV: bin = "div"; break; // TODO: idiv?
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

        // TODO signed variants 
        case VX_IR_OP_GT:  // "a", "b"
        case VX_IR_OP_GTE: // "a", "b"
        case VX_IR_OP_LT:  // "a", "b"
        case VX_IR_OP_LTE: // "a", "b"
        case VX_IR_OP_EQ:  // "a", "b"
        case VX_IR_OP_NEQ: // "a", "b"
            {
                vx_IrVar ov = op->outs[0].var;
                Location* o = varData[ov].location;
                assert(o);
                Location* a = as_loc(o->bytesWidth, *vx_IrOp_param(op, VX_IR_NAME_OPERAND_A));
                Location* b = as_loc(o->bytesWidth, *vx_IrOp_param(op, VX_IR_NAME_OPERAND_B));

                emiti_cmp(a, b, file);

                const char *cc;
                switch (op->id) {
                case VX_IR_OP_GT: cc = "a"; break;
                case VX_IR_OP_LT: cc = "b"; break;
                case VX_IR_OP_GTE: cc = "ae"; break;
                case VX_IR_OP_LTE: cc = "le"; break;
                case VX_IR_OP_EQ: cc = "e"; break;
                case VX_IR_OP_NEQ: cc = "ne"; break;

                default: assert(false); break;
                }

                if (!(op->next && vx_IrOp_var_used(op->next, ov) && (op->next->id == VX_IR_OP_CMOV || op->next->id == VX_IR_OP_CONDTAILCALL || op->next->id == VX_LIR_COND))) {
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

                    if (ty == VX_LIR_COND) {
                        vx_IrValue id = *vx_IrOp_param(op->next, VX_IR_NAME_ID);
                        vx_IrValue v = *vx_IrOp_param(op->next, VX_IR_NAME_COND);
                        if (v.type == VX_IR_VAL_VAR && v.var == ov) {
                            emiti_jump_cond(as_loc(8, id), cc, file);
                            return op->next->next;
                        }
                    }

                    if (ty == VX_IR_OP_CONDTAILCALL) {
                        vx_IrValue addr = *vx_IrOp_param(op->next, VX_IR_NAME_ADDR);
                        vx_IrValue v = *vx_IrOp_param(op->next, VX_IR_NAME_COND);
                        if (v.type == VX_IR_VAL_VAR && v.var == ov) {
                            emit_call_arg_load(op->next, file);
                            emiti_jump_cond(as_loc(8, addr), cc, file);
                            emit_call_ret_store(op->next, file);
                            return op->next->next;
                        }
                    }
                }
            } break;


        case VX_LIR_COND:            // "id", "cond": bool
            {
                vx_IrValue id = *vx_IrOp_param(op, VX_IR_NAME_ID);
                vx_IrValue cond = *vx_IrOp_param(op, VX_IR_NAME_COND);
                Location* loc = as_loc(1, cond);
                emiti_cmp0(loc, file);
                emiti_jump_cond(as_loc(8, id), "z", file);
            } break;

        case VX_IR_OP_CONDTAILCALL:  // "addr": int / fnref, "cond": bool
            {
                vx_IrValue addr = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
                vx_IrValue cond = *vx_IrOp_param(op, VX_IR_NAME_COND);
                Location* loc = as_loc(1, cond);
                emiti_cmp0(loc, file);
                emit_call_arg_load(op, file);
                emiti_jump_cond(as_loc(8, addr), "z", file);
            } break;

        case VX_IR_OP_CMOV:          // "cond": ()->bool, "then": value, "else": value
            {
                vx_IrValue cond = *vx_IrOp_param(op, VX_IR_NAME_COND);
                Location* loc = as_loc(1, cond);
                emiti_cmp0(loc, file);
                emit_condmove(op, "z", file);
            } break;

        case VX_LIR_OP_LABEL:        // "id"
            {
                vx_IrValue id = *vx_IrOp_param(op, VX_IR_NAME_ID);
                fprintf(file, ".l%zu:\n", id.id);
            } break;

        case VX_LIR_GOTO:            // "id"
            {
                Location* tg = as_loc(8, *vx_IrOp_param(op, VX_IR_NAME_ID));
                emiti_jump(tg, file);
            } break;

        case VX_IR_OP_CALL:          // "addr": int / fnref
            {
                vx_IrValue addr = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
                emit_call_arg_load(op, file);
                emiti_call(as_loc(8, addr), file);
                emit_call_ret_store(op, file);
            } break;

        case VX_IR_OP_TAILCALL:      // "addr": int / fnre  // TODO: stop inliner from inline addr because need fn type 
            {
                vx_IrValue addr = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
                emit_call_arg_load(op, file);
                emiti_jump(as_loc(8, addr), file);
            } break;

        default:
            assert(false);
            break;
    }

    return op->next;
}

void vx_cg_x86stupid_gen(vx_IrBlock* block, FILE* out) {
    fprintf(out, "%s:\n", block->name);

    assert(block->is_root);

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
    char * availableRegisters = fastalloc(availableRegistersCount);
    availableRegisters[0] = REG_R10.id;
    availableRegisters[1] = REG_R11.id;


    size_t anyArgsCount = block->ins_len;
    size_t anyCalledArgsCount = 0;
    // max arg len 
    for (vx_IrOp* op = block->first; op != NULL; op = op->next) {
        if (op->id == VX_IR_OP_CALL || op->id == VX_IR_OP_TAILCALL || op->id == VX_IR_OP_CONDTAILCALL) {
            vx_IrValue addr = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
            vx_IrTypeRef ty = vx_IrValue_type(block, addr);
            assert(ty.ptr != NULL);
            assert(ty.ptr->kind == VX_IR_TYPE_FUNC);

            vx_IrTypeFunc fn = ty.ptr->func;

            if (fn.args_len > anyCalledArgsCount) {
                anyCalledArgsCount = fn.args_len;
            }

            vx_IrTypeRef_drop(ty);
        }
    }
    if (anyCalledArgsCount)
        anyArgsCount = anyCalledArgsCount;

    if (anyArgsCount < 6) {
        size_t extraAv = 6 - anyArgsCount;
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

    printf("avail regs:\n");
    for (size_t i = 0; i < availableRegistersCount; i ++) {
        printf("- %s\n", RegLut[availableRegisters[i]]->name[3]);
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
        for (size_t i = anyCalledArgsCount; i < block->ins_len; i ++) {
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
            if (varId >= varsHotFirstLen) break;

            for (; varId < varsHotFirstLen; varId ++) {
                vx_IrVar var = varsHotFirst[varId];
                
                if (varData[var].type == NULL) continue;

                size_t size = vx_IrType_size(varData[var].type);
                if (size > 8) continue;
                
                if (block->as_root.vars[var].ever_placed) continue; 

                varData[var].location = gen_reg_var(widthToWidthWidth[size], availableRegisters[i]);
                break;
            }
            varId ++;
        }

        size_t stackOff = 0;
        for (; varId < varsHotFirstLen; varId ++) {
            vx_IrVar var = varsHotFirst[varId];
            if (varData[var].type == NULL) continue;
            size_t size = vx_IrType_size(varData[var].type);
            varData[var].location = gen_stack_var(size, stackOff);
            stackOff += size;
            anyPlaced = true;
        }

        free(varsHotFirst);
    }

    char argRegs[6] = { REG_RDI.id, REG_RSI.id, REG_RDX.id, REG_RCX.id, REG_R8.id, REG_R9.id };

    for (size_t i = 0; i < anyCalledArgsCount; i++) {
        Location* dest = varData[block->ins[i].var].location;
        Location* src = gen_reg_var(src->bytesWidth, argRegs[i]);
        emiti_move(src, dest, false, out);
    }

    for (size_t i = anyCalledArgsCount; i < block->ins_len; i ++) {
        vx_IrVar var = block->ins[i].var;
        vx_IrType* type = block->ins[i].type;
        varData[var].location = gen_reg_var(vx_IrType_size(type), argRegs[i]);
    }

    for (vx_IrVar i = 0; i < block->as_root.vars_len; i ++) {
        Location* loc = varData[i].location;
        const char * str = "(null)";
        if (loc != NULL) {
            switch (loc->type) {
            case LOC_REG:
                str = "reg";
                break;

            case LOC_MEM:
                str = "mem";
                break;
            
            default:
                str = "???";
                break;
            }
        }
        printf("var %zu: type %p , loc %s , size %zu , heat %zu\n", i, varData[i].type, str, loc ? loc->bytesWidth : 0, varData[i].heat);
    }

    if (anyPlaced)
        emiti_enter(out);

    vx_IrOp* op = block->first;
    vx_IrOp* prev = NULL;

    while (op != NULL) {
        vx_IrOp* new = emiti(prev, op, out);
        prev = op;
        op = new;
    }

    if (anyPlaced)
        emiti_leave(out);
    fputs("ret\n", out);

    free(varData);
    varData = NULL;
}

static Location* start_scratch_reg(size_t size, FILE* out) {
    assert(RegLut[SCRATCH_REG]->stored == NULL); // TODO 
    Location* loc = gen_reg_var(size, SCRATCH_REG);
    RegLut[SCRATCH_REG]->stored = loc;
    return loc;
}

static void end_scratch_reg(Location* loc, FILE* out) {
    assert(RegLut[SCRATCH_REG]->stored == loc);
    RegLut[SCRATCH_REG]->stored = NULL;
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

    if (loc->type == LOC_REG || loc->type == LOC_IMM || loc->type == LOC_EA) {
        Location* copy = loc_opt_copy(loc);
        copy->bytesWidth = size;
        return copy;
    }

    Location* prim = start_as_primitive(loc, out);

    return prim;
}

static void end_as_size(Location* as_size, Location* old, FILE* out) {
    if (as_size->type == LOC_REG && old->type != LOC_REG) {
        end_as_primitive(as_size, old, out);
    }
}
