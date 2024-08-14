#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    char * name;

    struct {
        bool     set;
        uint64_t opt_addr;
    } addr;

    bool export_label;
} ParsedLabel;

typedef struct Operand {
    enum {
        OPERAND_IMM,
        OPERAND_REG,
        OPERAND_LABEL,
        OPERAND_ADDRESS,
    } kind;

    union {
        struct {
            int64_t value;
        } imm;

        struct {
            const char * reg;
        } reg;

        struct {
            const char * name;
        } label;

        struct { // opt_base + opt_index * opt_multiplier
            size_t opt_bits_size; // or 0
            // operands can only be  OPERAND_LABEL, OPERAND_IMM and OPERAND_REG
            struct Operand * opt_base;
            bool opt_index_negative;
            struct Operand * opt_index;
            struct Operand * opt_multiplier;
        } address;
    } v;
} Operand;

typedef struct {
    char * name;
    Operand **operand_items;
    size_t    operand_len;
} ParsedInst;

typedef struct {
    char * name;
} ParsedDefine;

typedef struct {
    char * ifdef;
} ParsedCond;

typedef struct {
    enum {
        OP_INST,
        OP_LABEL,
        OP_DEFINE,
        OP_IFDEF,
        OP_IFNDEF,
        OP_ENDIF,
    } kind;

    union {
        ParsedInst   inst;
        ParsedLabel  label;
        ParsedDefine define;
        ParsedCond   conditional;
    } v;
} ParsedOp;

typedef struct {
    char * label;
} ParsedImport;

typedef struct {
    size_t sourceLine;
    char * msg;
} ParseError;

typedef struct {
    ParseError * items;
    size_t       len;
} ParseErrors;

typedef struct {
    ParseErrors errors;

    ParsedImport * imports_items;
    size_t         imports_len;

    ParsedOp * ops_items;
    size_t     ops_len;
} ParseRes;

typedef struct {
    void* asmTargetData;

    const char ** validregs_items; 
    size_t        validregs_len;
} ParseCtx;

void ParseRes_free(ParseRes);
void ParseRes_resolve(ParseRes); // TODO: local labels
void Parseres_printErrors(ParseRes, FILE*);
ParseRes assembler_parse(const char * source, ParseCtx ctx);

void bytesFromBits(const char * bits, uint8_t * out);
void bitsReplace(char * bits, char what, int64_t with);

typedef struct {
    const char * name;
    size_t location;
} AsmSym;

typedef struct {
    const char * name;
    uint8_t kind;
    size_t location;
} AsmUnresSym;

typedef struct {
    uint8_t * bytes_items;
    size_t    bytes_len;

    AsmUnresSym * unresolved_items;
    size_t        unresolved_len;

    AsmSym * exported_items;
    size_t   exported_len;
} AsmRes;

typedef struct {
    uint8_t * destbytes_items;
    size_t    destbytes_len;

    uint8_t unreskind;
    size_t  unresloc;

    size_t actualloc;
} LinkCtx;

typedef struct {
    const char * extensionString;
} ArchConfig;

typedef struct {
    ParseCtx (*prepare)(ArchConfig);
    AsmRes* (*assemble)(ParseRes, ParseCtx); // ParseRes is already resolved here
    void (*link)(LinkCtx, ParseCtx);
} AsmTarget;

extern AsmTarget etca_asmTarget;
