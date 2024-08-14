#include <assert.h>
#include <stdint.h>
#include "internal.h"
#include "../common/common.h"

typedef struct {
    vx_Target_ETCA__flags flags;
} Data;

static ParseCtx prepare(ArchConfig cfg) {
    Data* data = malloc(sizeof(Data));
    memset(data, 0, sizeof(Data));

    vx_Target_ETCA__parseAdditionalFlags(&data->flags, cfg.extensionString);

    ParseCtx res;
    res.asmTargetData = data;

    size_t regCount = 8;
    if (data->flags[vx_Target_ETCA_expandedReg]) {
        regCount = 16;
    }

    bool r8 = data->flags[vx_Target_ETCA_byte];
    bool r32 = data->flags[vx_Target_ETCA_dword];
    bool r64 = data->flags[vx_Target_ETCA_qword];

    size_t regMul = 1;
    if (r8) regMul ++;
    if (r32) regMul ++;
    if (r64) regMul ++;

    res.validregs_len = regCount * regMul;
    res.validregs_items = malloc(sizeof(const char *) * regCount * regMul);

    size_t base = 0;

    for (size_t i = 0; i < regCount; i ++) {
        char s[8];
        sprintf(s, "r%zu", i);
        res.validregs_items[base + i] = strdup(s);
    }
    base += regCount;

    if (r8) {
        for (size_t i = 0; i < regCount; i ++) {
            char s[8];
            sprintf(s, "rh%zu", i);
            res.validregs_items[base + i] = strdup(s);
        }
        base += regCount;
    }

    if (r32) {
        for (size_t i = 0; i < regCount; i ++) {
            char s[8];
            sprintf(s, "rd%zu", i);
            res.validregs_items[base + i] = strdup(s);
        }
        base += regCount;
    }

    if (r64) {
        for (size_t i = 0; i < regCount; i ++) {
            char s[8];
            sprintf(s, "rq%zu", i);
            res.validregs_items[base + i] = strdup(s);
        }
        base += regCount;
    }
    
    return res;
}

static int64_t parseReg(Operand* operand, size_t * sizeId) {
    assert(operand->kind == OPERAND_REG);
    const char * reg = operand->v.reg.reg;
    assert(reg[0] == 'r');
    reg ++;

    size_t sizeIdI = 1;
    if (reg[0] == 'h') {
        sizeIdI = 0;
        reg ++;
    } else if (reg[0] == 'd') {
        sizeIdI = 2;
        reg ++;
    } else if (reg[0] == 'q') {
        sizeIdI = 3;
        reg ++;
    }

    if (sizeId)
        *sizeId = sizeIdI;

    return (int64_t) atoi(reg);
}

// TODO: actually don't do this ! extend fake table gen to asm instructions because nicer to write assembler + free disasm

static void simpleOp(ParsedInst inst, uint8_t (*dest)[2], bool regRegOnly, bool regImmOnly, bool allowDiffSize, int opcode, int mm) {
    assert(inst.operand_len == 2);
    size_t aSizeId;
    size_t aRegId = parseReg(inst.operand_items[0], &aSizeId);

    Operand* opB = inst.operand_items[1];
    
    if (opB->kind == OPERAND_REG) {
        if (regImmOnly)
            perror("reg-reg mode used in op that only supports reg-imm");
    
        size_t bSizeId;
        size_t bRegId = parseReg(opB, &bSizeId);

        if (!allowDiffSize && aSizeId != bSizeId)
            perror("computation on two different sizes");

        if (aSizeId > bSizeId)
            bSizeId = aSizeId;

        char *bits = "00ssooooaaabbbmm";
        bitsReplace(bits, 's', bSizeId);
        bitsReplace(bits, 'o', opcode);
        bitsReplace(bits, 'a', aRegId);
        bitsReplace(bits, 'b', bRegId);
        bitsReplace(bits, 'm', mm);
        bytesFromBits(bits, *dest);
    }
    else if (opB->kind == OPERAND_IMM) {
        if (regRegOnly)
           perror("reg-imm mode used in op that only supports reg-reg");
    
        char *bits = "01ssooooaaaiiiii";
        bitsReplace(bits, 's', aSizeId);
        bitsReplace(bits, 'o', opcode);
        bitsReplace(bits, 'a', aRegId);
        bitsReplace(bits, 'i', opB->v.imm.value);
        bytesFromBits(bits, *dest);
    }
    else {
        perror("invalid second operand");
    }
}

static AsmRes* assemble(ParseRes parsed, ParseCtx ctx) { // parsed is resolved already 

}

static void link(LinkCtx link, ParseCtx ctx) {

}

AsmTarget etca_asmTarget = (AsmTarget) {
    .prepare = prepare,
    .assemble = assemble,
    .link = link,
};
