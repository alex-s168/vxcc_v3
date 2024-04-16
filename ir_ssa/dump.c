#include "ssa.h"

const char *ssaoptype_names[SSAOPTYPE_LEN] = {
    [SSA_OP_NOP] = "nop",
    [SSA_OP_IMM] = "imm",
    [SSA_OP_FLATTEN_PLEASE] = ".",
    
    [SSA_OP_REINTERPRET] = "reinterpret",
    [SSA_OP_ZEROEXT] = "zext",
    [SSA_OP_SIGNEXT] = "sext",
    [SSA_OP_TOFLT] = "toflt",
    [SSA_OP_FROMFLT] = "fromflt",
    [SSA_OP_BITCAST] = "bitcast",

    [SSA_OP_ADD] = "add",
    [SSA_OP_SUB] = "sub",
    [SSA_OP_MUL] = "mul",
    [SSA_OP_DIV] = "div",
    [SSA_OP_MOD] = "mod",

    [SSA_OP_GT] = "gt",
    [SSA_OP_GTE] = "gte",
    [SSA_OP_LT] = "lt",
    [SSA_OP_LTE] = "lte",
    [SSA_OP_EQ] = "eq",
    [SSA_OP_NEQ] = "neq",

    [SSA_OP_NOT] = "not",
    [SSA_OP_AND] = "and",
    [SSA_OP_OR] = "or",

    [SSA_OP_BITWISE_NOT] = "bwnot",
    [SSA_OP_BITWISE_AND] = "bwand",
    [SSA_OP_BITIWSE_OR] = "bwor",

    [SSA_OP_SHL] = "shl",
    [SSA_OP_SHR] = "shr",

    [SSA_OP_FOR] = "for",
    [SSA_OP_INFINITE] = "infinite",
    [SSA_OP_WHILE] = "while",
    [SSA_OP_CONTINUE] = "continue",
    [SSA_OP_BREAK] = "break",

    [SSA_OP_FOREACH] = "foreach",
    [SSA_OP_FOREACH_UNTIL] = "foreach_until",
    [SSA_OP_REPEAT] = "repeat",
    [CIR_OP_CFOR] = "cfor",

    [SSA_OP_IF] = "if"
};

const char *ssaname_str[] = {
    [SSA_NAME_OPERAND_A] = "a",
    [SSA_NAME_OPERAND_B] = "b",

    [SSA_NAME_BLOCK] = "block",
    [SSA_NAME_VALUE] = "val",
    [SSA_NAME_COND] = "cond",

    [SSA_NAME_COND_THEN] = "then",
    [SSA_NAME_COND_ELSE] = "else",

    [SSA_NAME_LOOP_DO] = "do",
    [SSA_NAME_LOOP_START] = "start",
    [SSA_NAME_LOOP_ENDEX] = "endex",
    [SSA_NAME_LOOP_STRIDE] = "stride",

    [SSA_NAME_ALTERNATIVE_A] = "a",
    [SSA_NAME_ALTERNATIVE_B] = "b",
};

void ssavalue_dump(SsaValue value, FILE *out, const size_t indent) {
    switch (value.type) {
        case SSA_VAL_IMM_INT: {
            fprintf(out, "%lld", value.imm_int);
        }
        break;

        case SSA_VAL_IMM_FLT: {
            fprintf(out, "%f", value.imm_flt);
        }
        break;

        case SSA_VAL_VAR: {
            fprintf(out, "%%%zu", value.var);
        }
        break;

        case SSA_VAL_BLOCK: {
            const SsaBlock *block = value.block;

            fputc('(', out);
            for (size_t i = 0; i < block->ins_len; i ++) {
                if (i > 0)
                    fputc(',', out);
                const SsaVar in = block->ins[i];
                fprintf(out, "%%%zu", in);
            }
            fputs("){\n", out);

            for (size_t i = 0; i < block->ops_len; i ++) {
                ssaop_dump(block->ops + i, out, indent + 1);
            }

            if (block->outs_len > 0) {
                for (size_t j = 0; j < indent + 1; j ++)
                    fputs("  ", out);

                fputs("^ ", out);

                for (size_t i = 0; i < block->outs_len; i ++) {
                    if (i > 0)
                        fputc(',', out);
                    fprintf(out, "%%%zu", block->outs[i]);
                }

                fputc('\n', out);
            }

            for (size_t j = 0; j < indent; j ++)
                fputs("  ", out);

            fputc('}', out);
        }
        break;
    }
}

void ssaop_dump(const SsaOp *op, FILE *out, size_t indent) {
    for (size_t j = 0; j < indent; j ++)
        fputs("  ", out);

    for (size_t j = 0; j < op->outs_len; j ++) {
        if (j > 0)
            fputc(',', out);
        const SsaTypedVar var = op->outs[j];
        fprintf(out, "%s %%%zu", var.type, var.var);
    }

    if (op->outs_len > 0)
        fputs(" = ", out);

    fprintf(out, "%s ", ssaoptype_names[op->id]);

    if (op->types_len > 0) {
        fputc('<', out);

        for (size_t i = 0; i < op->types_len; i ++) {
            if (i > 0)
                fputc(',', out);
            fputs(op->types[i], out);
        }

        fputc('>', out);
    }

    for (size_t j = 0; j < op->params_len; j ++) {
        if (j > 0)
            fputc(' ', out);
        const SsaNamedValue param = op->params[j];
        fprintf(out, "%s=", ssaname_str[param.name]);
        ssavalue_dump(param.val, out, indent);
    }

    fputs(";\n", out);
}

void ssablock_dump(const SsaBlock *block, FILE *out, const size_t indent) {
    for (size_t i = 0; i < indent; i ++)
        fputs("  ", out);

    fputs("BLOCK", out);
    for (size_t i = 0; i < block->ins_len; i ++) {
        const SsaVar in = block->ins[i];
        fprintf(out, " %%%zu", in);
    }
    fputc('\n', out);

    for (size_t i = 0; i < block->ops_len; i ++) {
        ssaop_dump(block->ops + i, out, indent + 1);
    }

    for (size_t i = 0; i < indent; i ++)
        fputs("  ", out);

    fputs("return", out);
    for (size_t i = 0; i < block->outs_len; i ++) {
        const SsaVar var = block->outs[i];
        fprintf(out, " %%%zu", var);
    }
    fputc('\n', out);
}
