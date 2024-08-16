#include <stdint.h>
#include <assert.h>
#include "internal.h"

typedef enum : uint8_t {
    LINKREF_
} LinkRefKind;

static ParseCtx prepare(ArchConfig cfg) {

}

static uint8_t* appendBytes(AsmRes* res, size_t count) {
    res->bytes_items = realloc(res->bytes_items, (res->bytes_len + count));
    uint8_t* ptr = &res->bytes_items[res->bytes_len];
    res->bytes_len += count;
    return ptr;
}

static uint8_t parseReg(Operand* operand, bool * is_pc) {
    assert(operand->kind == OPERAND_REG);
    const char * reg = operand->v.reg.reg;
    
    assert(reg[2] == '\0');

    if (reg[0] == 'r') {
        char id = reg[1] - '0';
        assert(id >= 0 && id <= 2);
        if (is_pc)
            *is_pc = false;
        return id;
    }

    if (reg[0] == 'c') {
        assert(reg[1] == '1');
        if (is_pc)
            *is_pc = false;
        return 0b11;
    }

    assert(reg[0] == 'p' && reg[1] == 'c');
    if (is_pc)
        *is_pc = true;
    return 0b11;
}

static AsmRes* assemble(ParseRes parsed, ParseCtx ctx) { // ParseRes is already resolved here
    AsmRes* res = calloc(sizeof(AsmRes), 1);

    for (size_t instrId = 0; instrId < parsed.ops_len; instrId ++) {
        ParsedOp op = parsed.ops_items[instrId];

        if (op.kind == OP_LABEL) {
            assert(!op.v.label.addr.set); // TODO 

            res->defined_items = realloc(res->defined_items, sizeof(AsmSym) * (res->bytes_len + 1));
            res->defined_items[res->defined_len ++] = (AsmSym) {
                .location = res->bytes_len,
                .name = op.v.label.name,
                .exported = op.v.label.export_label,
            };
        }
        else {
            assert(op.kind == OP_INST);
            ParsedInst inst = op.v.inst;

            if (strcmp(inst.name, "adc") == 0) {
                assert(inst.operand_len == 2); 
                char bits[] = "0000ddss";
                bool isPc;
                bitsReplace(bits, 'd', parseReg(inst.operand_items[0], &isPc));
                assert(!isPc);
                bitsReplace(bits, 's', parseReg(inst.operand_items[1], &isPc));
                assert(!isPc);
                bytesFromBits(bits, appendBytes(res, 1));
            }
            else if (strcmp(inst.name, "not") == 0) {
                assert(inst.operand_len == 2); 
                char bits[] = "0001ddss";
                bool isPc;
                bitsReplace(bits, 'd', parseReg(inst.operand_items[0], &isPc));
                assert(!isPc);
                bitsReplace(bits, 's', parseReg(inst.operand_items[1], &isPc));
                assert(!isPc);
                bytesFromBits(bits, appendBytes(res, 1));
            }
            else if (strcmp(inst.name, "ec9") == 0) {
                assert(inst.operand_len == 1); 
                char bits[] = "001000ss";
                bool isPc;
                bitsReplace(bits, 's', parseReg(inst.operand_items[0], &isPc));
                assert(!isPc);
                bytesFromBits(bits, appendBytes(res, 1));
            }
            else if (strcmp(inst.name, "fwc") == 0) {
                assert(inst.operand_len == 1); 
                char bits[] = "001100dd";
                bool isPc;
                bitsReplace(bits, 'd', parseReg(inst.operand_items[0], &isPc));
                assert(!isPc);
                bytesFromBits(bits, appendBytes(res, 1));
            }
            else if (strcmp(inst.name, "tst") == 0) {
                assert(inst.operand_len == 1); 
                char bits[] = "010000ss";
                bool isPc;
                bitsReplace(bits, 's', parseReg(inst.operand_items[0], &isPc));
                assert(!isPc);
                bytesFromBits(bits, appendBytes(res, 1));
            }
            else if (strcmp(inst.name, "ltu") == 0) {
                assert(inst.operand_len == 2); 
                char bits[] = "0101aabb";
                bool isPc;
                bitsReplace(bits, 'a', parseReg(inst.operand_items[0], &isPc));
                assert(!isPc);
                bitsReplace(bits, 'b', parseReg(inst.operand_items[1], &isPc));
                assert(!isPc);
                bytesFromBits(bits, appendBytes(res, 1));
            }
            else if (strcmp(inst.name, "orr") == 0) {
                assert(inst.operand_len == 2); 
                char bits[] = "0110ddss";
                bool isPc;
                bitsReplace(bits, 'd', parseReg(inst.operand_items[0], &isPc));
                assert(!isPc);
                bitsReplace(bits, 's', parseReg(inst.operand_items[1], &isPc));
                assert(!isPc);
                bytesFromBits(bits, appendBytes(res, 1));
            }
            else if (strcmp(inst.name, "swp") == 0) {
                assert(inst.operand_len == 1); 
                char bits[] = "011100rr";
                bool isPc;
                bitsReplace(bits, 'r', parseReg(inst.operand_items[0], &isPc));
                assert(!isPc);
                bytesFromBits(bits, appendBytes(res, 1));
            }
            else if (strcmp(inst.name, "stb") == 0) {
                assert(inst.operand_len == 1); 
                char bits[] = "100000dd";
                bool isPc;
                bitsReplace(bits, 'd', parseReg(inst.operand_items[0], &isPc));
                assert(!isPc);
                bytesFromBits(bits, appendBytes(res, 1));
            }
            else if (strcmp(inst.name, "ldb") == 0) {
                assert(inst.operand_len == 1); 
                char bits[] = "100001ss";
                bool isPc;
                bitsReplace(bits, 's', parseReg(inst.operand_items[0], &isPc));
                assert(!isPc);
                bytesFromBits(bits, appendBytes(res, 1));
            }
            else if (strcmp(inst.name, "clc") == 0) {
                assert(inst.operand_len == 0);
                *appendBytes(res, 1) = 0b10001000;
            }
            else if (strcmp(inst.name, "inv") == 0) {
                assert(inst.operand_len == 0);
                *appendBytes(res, 1) = 0b10001001;
            }
            else {
                assert(/* unknown instr */ false);
            }
        }
    }

    return res;
}

static void link(LinkCtx linkCtx, ParseCtx ctx) {

}

AsmTarget ua16_asmTarget = {
    .prepare = prepare,
    .assemble = assemble,
    .link = link
};
