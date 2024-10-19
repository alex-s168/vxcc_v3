#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"


typedef struct {
    vx_IrName name;
    OperandType type;
    union {
        char * placeholder;
        double imm_flt;
    } v;
} Operand;

typedef struct {
    struct {
        char ** items;
        uint32_t count;
    } outputs;

    const char * name;
    uint32_t name_len;

    struct {
        Operand * items;
        uint32_t    count;
    } operands;
} Operation;

typedef struct {
    struct {
        Operation * items;
        uint32_t count;
    } operations;
} Pattern;

static vx_IrName parse_name(const char * src, uint32_t srcLen)
{
    for (vx_IrName i = 0; i < VX_IR_NAME__LAST; i ++)
    {
        const char * nam = vx_IrName_str[i];
        uint32_t nam_len = strlen(nam);
        if (nam_len != srcLen) continue;
        if (!memcmp(nam, src, nam_len)) {
            return i;
        }
    }
    assert(false && "could not parse ir instr param name");
    return VX_IR_NAME__LAST; // unreachable
}

static Operation parse_op(char * line, uint32_t line_len) 
{
    Operation op = (Operation) {
        .outputs.items = NULL,
        .outputs.count = 0,

        .operands.items = NULL,
        .operands.count = 0
    };

    while (isspace(*line)) line ++;

    char * assign = strchr(line, '=');
    if (assign) {
        while (true)
        {
            while (isspace(*line)) line ++;

            char * comma = strchr(line, ',');
            char * last = comma ? comma : assign;
            uint32_t count = last - line;

            char* buf = malloc(count + 1);
            memcpy(buf, line, count);
            buf[count] = '\0';
            op.outputs.items = realloc(op.outputs.items, (op.outputs.count + 1) * sizeof(char*));
            op.outputs.items[op.outputs.count ++] = buf;

            if (comma > assign)
                break;

            if (comma) 
                line = comma + 1;
            else  
                break;
        }

        line = assign + 1;
    }

    while (isspace(*line)) line ++;

    const char * name_begin = line;
    uint32_t name_len = 0;
    while (isalnum(*line)) {
        name_len ++;
        line ++;
    }
    op.name = name_begin;
    op.name_len = name_len;

    while (isspace(*line)) line ++;

    assert(*line == '('); line ++;

    char * close = strchr(line, ')'); assert(close);

    while (true)
    {
        while (isspace(*line)) line ++;

        vx_IrName opnam;
        {
            char * eqsign = strchr(line, '=');
            assert(eqsign);
            assert(eqsign < close);
            *eqsign = '\0';
            char * space = strchr(line, ' ');
            if (space) *space = '\0';
            opnam = parse_name(line, strlen(line));
            line = eqsign;

        }
        while (isspace(*line)) line ++;

        char * comma = strchr(line, ',');
        char * last = comma ? comma : (close - 1);
        uint32_t count = last - line;

        char* buf = malloc(count + 1);
        memcpy(buf, line, count);
        buf[count] = '\0';

        Operand operand;
        operand.name = opnam;
        if (isdigit(*buf)) {
            operand.type = OPERAND_TYPE_IMM_FLT;
            sscanf(buf, "%lf", &operand.v.imm_flt);
            free(buf);
        }
        else {
            operand.type = OPERAND_TYPE_PLACEHOLDER;
            operand.v.placeholder = buf;
        }

        op.operands.items = realloc(op.operands.items, sizeof(Operand) * (op.operands.count + 1));
        op.operands.items[op.operands.count ++] = operand;

        line = last + 1;
        if (!comma) 
            break;
    }

    while (isspace(*line)) line ++;

    assert(*line == ')'); line ++; assert(*line == '\0');

    return op;
}

static uint32_t placeholder(const char *** all_placeholders, uint32_t * all_placeholders_len, const char * name)
{
    for (uint32_t i = 0; i < *all_placeholders_len; i ++) {
        if (strcmp((*all_placeholders)[i], name) == 0) {
            return i;
        }
    }
    uint32_t plLen = *all_placeholders_len ++;
    *all_placeholders = realloc(*all_placeholders, sizeof(const char *) * (*all_placeholders_len));
    (*all_placeholders)[plLen] = name;
    return plLen;
}

static CompPattern compile(Pattern pat)
{
    const char ** all_placeholders = NULL;
    uint32_t all_placeholders_len = 0;

    CompPattern res;
    res.items = malloc(sizeof(CompOperation) * pat.operations.count);
    res.count = pat.operations.count;

    for (uint32_t i = 0; i < pat.operations.count; i ++) 
    {
        CompOperation* cop = &res.items[i];
        Operation* op = &pat.operations.items[i];

        if (strcmp(op->name, "...") == 0)
        {
            cop->is_any = true;
            assert(op->outputs.count == 0);
            assert(op->operands.count == 0);
        }
        else  
        {
            cop->is_any = false;

            bool found = false;
            for (uint32_t i = 0; i < vx_IrOpType__len; i ++)
            {
                const char *cmp = vx_IrOpType__entries[i].debug.a;
                if (strlen(cmp) != op->name_len) continue;
                if (memcmp(cmp, op->name, op->name_len) == 0) {
                    found = true;
                    cop->specific.op_type = i;
                    break;
                }
            }
            if (!found) {
                ((char*) op->name)[op->name_len] = '\0';
                fprintf(stderr, "No IrOpType \"%s\" found!\n", op->name);
                exit(1);
            }

            cop->specific.outputs.items = malloc(sizeof(uint32_t) * op->outputs.count);
            cop->specific.outputs.count = op->outputs.count;
            for (uint32_t i = 0; i < op->outputs.count; i ++) 
            {
                cop->specific.outputs.items[i] = placeholder(&all_placeholders, &all_placeholders_len, op->outputs.items[i]);
                free(op->outputs.items[i]);
            }
            free(op->outputs.items);

            cop->specific.operands.items = malloc(sizeof(CompOperand) * op->operands.count);
            cop->specific.operands.count = op->operands.count;
            for (uint32_t i = 0; i < op->operands.count; i ++)
            {
                CompOperand* co = &cop->specific.operands.items[i];
                co->name = op->operands.items[i].name;
                if (op->operands.items[i].type == OPERAND_TYPE_IMM_FLT) {
                    double val = op->operands.items[i].v.imm_flt;
                    if (val - (long long int)val == 0) {
                        co->type = OPERAND_TYPE_IMM_INT;
                        co->v.imm_int = (long long int) val;
                    } else {
                        co->type = OPERAND_TYPE_IMM_FLT;
                        co->v.imm_flt = val;
                    }
                } else {
                    co->type = OPERAND_TYPE_PLACEHOLDER;
                    co->v.placeholder = placeholder(&all_placeholders, &all_placeholders_len, op->operands.items[i].v.placeholder);
                    free(op->operands.items[i].v.placeholder);
                }
            }
            free(op->operands.items);
        }
    }
    free(pat.operations.items);

    res.placeholders_count = all_placeholders_len;
    res.placeholders = malloc(sizeof(germanstr) * res.placeholders_count);

    for (uint32_t i = 0; i < res.placeholders_count; i ++)
        res.placeholders[i] = germanstr_cstrdup(all_placeholders[i]);

    free(all_placeholders);
    return res;
}

/**
 * example pattern:
 * \code
 * S = sub(B, C)
 * A = add(S, C)
 * \endcode 
 *
 * example pattern:
 * \code 
 * B = load(A)
 * B = add(B, 1)
 * store(A, B)
 * \endcode
 */
CompPattern Pattern_compile(const char * source)
{
    Pattern out;
    out.operations.items = NULL;
    out.operations.count = 0;

    char** linebufs = NULL;

    while (*source)
    {
        const char * nt_ptr = strchr(source, '\n');
        if (nt_ptr == NULL) nt_ptr = source + strlen(source);
        uint32_t line_len = nt_ptr - source;
        char* linebuf = malloc(line_len + 1);
        memcpy(linebuf, source, line_len+1);
        linebuf[line_len] = '\0';
        Operation op = parse_op(linebuf, line_len);
        out.operations.items = realloc(out.operations.items, (out.operations.count + 1) * sizeof(Operation));
        linebufs = realloc(linebufs, sizeof(char*) * (out.operations.count + 1));
        linebufs[out.operations.count] = linebuf;
        out.operations.items[out.operations.count ++] = op;
        source += line_len;
        if (!*source) break;
        source ++;
    }

    CompPattern c = compile(out);

    for (uint32_t i = 0; i < out.operations.count; i ++)
        free(linebufs[i]);
    free(linebufs);

    return c;
}

void Pattern_free(CompPattern p)
{
    for (uint32_t i = 0; i < p.count; i ++)
    {
        if (!p.items[i].is_any)
        {
            free(p.items[i].specific.operands.items);
            free(p.items[i].specific.outputs.items);
        }
    }
    free(p.items);
    for (uint32_t i = 0; i < p.placeholders_count; i ++)
        germanstr_libcfree(p.placeholders[i]);
    free(p.placeholders);
}

uint32_t Pattern_placeholderId(CompPattern pat, const char * name)
{
    germanstr gname = germanstr_fromc((char*) name);
    for (uint32_t i = 0; i < pat.placeholders_count; i ++)
        if (germanstr_eq(pat.placeholders[i], gname))
            return i;
    assert(false); return 0;
}
