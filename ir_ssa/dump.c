#include "ssa.h"

const char *ssaoptype_names[SSAOPTYPE_LEN] = {
    "nop",
    "imm",

    "reinterpret",
    "zext",
    "sext",
    "toflt",
    "fromflt",
    "bitcast",

    "add",
    "sub",
    "mul",
    "div",
    "mod",

    "gt",
    "gte",
    "lt",
    "lte",
    "eq",
    "neq",

    "not",
    "and",
    "or",

    "bwnot",
    "bwand",
    "bwor",

    "shl",
    "shr",

    "for",
    "infinite",
    "while",
    "continue",
    "break",

    "foreach",
    "foreach_until",
    "repeat",
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
        fprintf(out, "%s=", param.name);
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
