#include <stdarg.h>
#include <ctype.h>
#include "internal.h"

static void errorImpl(ParseErrors * errors, size_t sourceLine, const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    static char buf[256];
    vsprintf(buf, fmt, args);
    va_end(args);

    ParseError err;
    err.msg = strdup(buf);
    err.sourceLine = sourceLine;
    errors->items = realloc(errors->items, sizeof(ParseError) * (errors->len + 1));
    errors->items[errors->len ++] = err;
}

#define error(...) errorImpl(errors, sourceLine, __VA_ARGS__)

static bool strstartsw(const char * str, const char * other) {
    size_t strLen = strlen(str);
    size_t otherLen = strlen(other);
    
    if (strLen < otherLen)
        return false;

    return memcmp(str, other, sizeof(char) * otherLen) == 0;
}

static char * strndup(const char * str, size_t maxlen) {
    size_t sl = strlen(str);
    if (maxlen < sl)
        sl = maxlen;
    sl ++; // nt 

    char * buf = malloc(sl * sizeof(char));
    if (buf == NULL) return NULL;
    memcpy(buf, str, sl * sizeof(char));
    return buf;
}

static bool parseTk(const char * * source, const char * other) {
    if (strstartsw(*source, other)) {
        *source += strlen(other);
        return true;
    }
    return false;
}

static void parseWhite(const char * * source) {
    while (isspace(**source))
        *source += 1;
}

static char* parseTillSep(const char * * source) {
    const char * begin = *source;
    size_t count = 0;

    while (**source != '\0' && **source != ' ') {
        *source += 1;
        count += 1;
    }

    return strndup(begin, count);
}

static Operand* parseOperand(const char * * source, ParseErrors * errors, size_t sourceLine, ParseCtx ctx) {
    if ((*source)[0] == '\0') {
        error("expected operand!");
        return NULL;
    }

    parseWhite(source);

    size_t bitsSize = 0;
    if (parseTk(source, "bit "))
        bitsSize = 1;
    else if (parseTk(source, "nibble "))
        bitsSize = 4;
    else if (parseTk(source, "byte "))
        bitsSize = 8;
    else if (parseTk(source, "word "))
        bitsSize = 16;
    else if (parseTk(source, "dword "))
        bitsSize = 32;
    else if (parseTk(source, "qword "))
        bitsSize = 64;
    else if (parseTk(source, "xword "))
        bitsSize = 128;
    else if (parseTk(source, "yword "))
        bitsSize = 256;
    else if (parseTk(source, "zword "))
        bitsSize = 512;

    if (bitsSize != 0)
        parseWhite(source);

    if (parseTk(source, "[")) {
        Operand* a = parseOperand(source, errors, sourceLine, ctx);
        if (a == NULL) {
            error("expected operand in memory address");
            return NULL;
        }

        parseWhite(source);

        Operand* b = NULL;
        bool b_neg = false;
        if (parseTk(source, "+")) {
            b = parseOperand(source, errors, sourceLine, ctx);
            if (b == NULL) {
                error("expected operand in memory address offset");
                return NULL;
            }
            parseWhite(source);
        } else if (parseTk(source, "-")) {
            b_neg = true;
            b = parseOperand(source, errors, sourceLine, ctx);
            if (b == NULL) {
                error("expected operand in memory address offset");
                return NULL;
            }
            parseWhite(source);
        }

        Operand* c = NULL;
        if (parseTk(source, "*")) {
            c = parseOperand(source, errors, sourceLine, ctx);
            if (c == NULL) {
                error("expected operand in memory address multiplier");
                return NULL;
            }
            parseWhite(source);
        }
        
        if (!parseTk(source, "]")) {
            error("expected square brackets close after memory address");
            return NULL;
        }

        Operand* res = malloc(sizeof(Operand));
        res->kind = OPERAND_ADDRESS;
        res->v.address.opt_bits_size = bitsSize;

        if (b == NULL && c != NULL) {
            res->v.address.opt_base = NULL;
            res->v.address.opt_index = a;
            res->v.address.opt_index_negative = false;
            res->v.address.opt_multiplier = c; 
        }
        else {
            res->v.address.opt_base = a; 
            res->v.address.opt_index_negative = b_neg;
            res->v.address.opt_index = b;
            res->v.address.opt_multiplier = c;
        }

        return res;
    }

    int radix = 10;

    if (parseTk(source, "0x"))
        radix = 16;
    else if (parseTk(source, "0b"))
        radix = 2;
    else if (parseTk(source, "0o"))
        radix = 8;

    char* num_end;
    uint64_t num = strtoll(*source, &num_end, radix);

    if (num_end != *source) {
        *source = num_end;

        Operand* res = malloc(sizeof(Operand));
        res->kind = OPERAND_IMM;
        res->v.imm.value = num;
        return res;
    }

    if (radix != 10) {
        error("expected number after radix %zu", radix);
        return NULL;
    }

    for (size_t i = 0; i < ctx.validregs_len; i ++) {
        const char * reg = ctx.validregs_items[i];
        if (parseTk(source, reg)) {
            Operand* res = malloc(sizeof(Operand));
            res->kind = OPERAND_REG;
            res->v.reg.reg = reg;
            return res;
        }
    }

    char * label = parseTillSep(source);
    if (*label == '\0') {
        error("expected operand");
        return NULL;
    }

    Operand* res = malloc(sizeof(Operand));
    res->kind = OPERAND_LABEL;
    res->v.label.name = label;
    return res;
}

static void parseLine(char * line, ParseRes* res, ParseCtx ctx, size_t lineId) {
    parseWhite((const char **) &line);

    char * hashtag = strchr(line, '#');
    if (hashtag)
        *hashtag = '\0';

    if (*line == '\0')
        return;

    res->ops_items = realloc(res->ops_items, (res->ops_len + 1) * sizeof(ParsedOp));
    ParsedOp* op = &res->ops_items[res->ops_len ++];

    char * colon = strchr(line, ':');
    if (colon) {
        *colon = '\0';

        op->kind = OP_LABEL;

        if (parseTk((const char **) &line, "export "))
            op->v.label.export_label = true;
        else 
            op->v.label.export_label = false;

        op->v.label.addr.set = false;
        op->v.label.name = line;

        return;
    }

    char * inst = parseTillSep((const char **) &line);

    op->kind = OP_INST;
    op->v.inst.name = inst;
    op->v.inst.operand_items = NULL;
    op->v.inst.operand_len = 0;

    do {
        parseWhite((const char **) &line);
        if (*line == '\0')
            break;

        Operand* arg = parseOperand((const char **) &line, &res->errors, lineId, ctx);

        op->v.inst.operand_items = realloc(op->v.inst.operand_items, (op->v.inst.operand_len + 1) * sizeof(Operand));
        op->v.inst.operand_items[op->v.inst.operand_len ++] = arg;

        parseWhite((const char **) &line);

        if (*line == '\0')
            break;

        if (!parseTk((const char **) &line, ",")) {
            errorImpl(&res->errors, lineId, "expected comma before next operand");
            return;
        }
    } while (true);
}

ParseRes assembler_parse(const char * source, ParseCtx ctx) {
    ParseRes result;
    result.errors.items = NULL; 
    result.errors.len = 0;
    result.ops_items = NULL;
    result.ops_len = 0;
    result.imports_items = NULL;
    result.imports_len = 0;

    size_t lineId = 0;
    while (*source) {
        const char * term = strchr(source, '\n');
        if (term == NULL)
            term = source + strlen(source);
        size_t count = term - source - 1;

        char * line = strndup(term, count);
        parseLine(line, &result, ctx, lineId);
        free(line);

        lineId ++;
    }

    return result;
}
