#include "ir.h"

const char *vx_IrOpType_names[SSAOPTYPE_LEN] = {
    [VX_IR_OP_NOP] = "nop",
    [VX_IR_OP_IMM] = "imm",
    [VX_IR_OP_FLATTEN_PLEASE] = ".",
    
    [VX_IR_OP_REINTERPRET] = "reinterpret",
    [VX_IR_OP_ZEROEXT] = "zext",
    [VX_IR_OP_SIGNEXT] = "sext",
    [VX_IR_OP_TOFLT] = "toflt",
    [VX_IR_OP_FROMFLT] = "fromflt",
    [VX_IR_OP_BITCAST] = "bitcast",

    [VX_IR_OP_LOAD] = "load",
    [VX_IR_OP_LOAD_VOLATILE] = "load-v",
    [VX_IR_OP_STORE] = "store",
    [VX_IR_OP_STORE_VOLATILE] = "store-v",
    [VX_IR_OP_PLACE] = "place",
    
    [VX_IR_OP_ADD] = "add",
    [VX_IR_OP_SUB] = "sub",
    [VX_IR_OP_MUL] = "mul",
    [VX_IR_OP_DIV] = "div",
    [VX_IR_OP_MOD] = "mod",

    [VX_IR_OP_GT] = "gt",
    [VX_IR_OP_GTE] = "gte",
    [VX_IR_OP_LT] = "lt",
    [VX_IR_OP_LTE] = "lte",
    [VX_IR_OP_EQ] = "eq",
    [VX_IR_OP_NEQ] = "neq",

    [VX_IR_OP_NOT] = "not",
    [VX_IR_OP_AND] = "and",
    [VX_IR_OP_OR] = "or",

    [VX_IR_OP_BITWISE_NOT] = "bwnot",
    [VX_IR_OP_BITWISE_AND] = "bwand",
    [VX_IR_OP_BITIWSE_OR] = "bwor",

    [VX_IR_OP_SHL] = "shl",
    [VX_IR_OP_SHR] = "shr",

    [VX_IR_OP_FOR] = "for",
    [VX_IR_OP_INFINITE] = "infinite",
    [VX_IR_OP_WHILE] = "while",
    [VX_IR_OP_CONTINUE] = "continue",
    [VX_IR_OP_BREAK] = "break",

    [VX_IR_OP_FOREACH] = "foreach",
    [VX_IR_OP_FOREACH_UNTIL] = "foreach_until",
    [VX_IR_OP_REPEAT] = "repeat",
    [VX_CIR_OP_CFOR] = "cfor",

    [VX_IR_OP_IF] = "if",

    [VX_LIR_OP_LABEL] = "label",
    [VX_LIR_GOTO] = "goto",
    [VX_LIR_COND] = "cond",
};

const char *vx_IrName_str[] = {
    [VX_IR_NAME_OPERAND_A] = "a",
    [VX_IR_NAME_OPERAND_B] = "b",

    [VX_IR_NAME_BLOCK] = "block",
    [VX_IR_NAME_VALUE] = "val",
    [VX_IR_NAME_ADDR] = "addr",
    [VX_IR_NAME_COND] = "cond",
    [VX_IR_NAME_VAR] = "var",

    [VX_IR_NAME_COND_THEN] = "then",
    [VX_IR_NAME_COND_ELSE] = "else",

    [VX_IR_NAME_LOOP_DO] = "do",
    [VX_IR_NAME_LOOP_START] = "start",
    [VX_IR_NAME_LOOP_ENDEX] = "endex",
    [VX_IR_NAME_LOOP_STRIDE] = "stride",

    [VX_IR_NAME_ALTERNATIVE_A] = "a",
    [VX_IR_NAME_ALTERNATIVE_B] = "b",

    [VX_IR_NAME_ID] = "id",
};

void vx_IrValue_dump(vx_IrValue value, FILE *out, const size_t indent) {
    switch (value.type) {
        case VX_IR_VAL_ID: {
            fprintf(out, "$%zu", value.id);
        }
        break;

        case VX_IR_VAL_UNINIT: {
            fprintf(out, "uninit");
        }
        break;

        case VX_IR_VAL_TYPE: {
            fprintf(out, "type %s", value.ty->debugName);
        }
        break;
        
        case VX_IR_VAL_IMM_INT: {
            fprintf(out, "%lld", value.imm_int);
        }
        break;

        case VX_IR_VAL_IMM_FLT: {
            fprintf(out, "%f", value.imm_flt);
        }
        break;

        case VX_IR_VAL_VAR: {
            fprintf(out, "%%%zu", value.var);
        }
        break;

        case VX_IR_VAL_BLOCK: {
            const vx_IrBlock *block = value.block;

            fputc('(', out);
            for (size_t i = 0; i < block->ins_len; i ++) {
                if (i > 0)
                    fputc(',', out);
                const vx_IrVar in = block->ins[i];
                fprintf(out, "%%%zu", in);
            }
            fputs("){\n", out);

            for (size_t i = 0; i < block->ops_len; i ++) {
                vx_IrOp_dump(block->ops + i, out, indent + 1);
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

void vx_IrOp_dump(const vx_IrOp *op, FILE *out, size_t indent) {
    for (size_t j = 0; j < indent; j ++)
        fputs("  ", out);

    for (size_t j = 0; j < op->outs_len; j ++) {
        if (j > 0)
            fputc(',', out);
        const vx_IrTypedVar var = op->outs[j];
        fprintf(out, "%s %%%zu", var.type->debugName, var.var);
    }

    if (op->outs_len > 0)
        fputs(" = ", out);

    fprintf(out, "%s ", vx_IrOpType_names[op->id]);

    for (size_t j = 0; j < op->params_len; j ++) {
        if (j > 0)
            fputc(' ', out);
        const vx_IrNamedValue param = op->params[j];
        fprintf(out, "%s=", vx_IrName_str[param.name]);
        vx_IrValue_dump(param.val, out, indent);
    }

    fputs(";\n", out);
}

void vx_IrBlock_dump(const vx_IrBlock *block, FILE *out, const size_t indent) {
    for (size_t i = 0; i < indent; i ++)
        fputs("  ", out);

    fputs("BLOCK", out);
    for (size_t i = 0; i < block->ins_len; i ++) {
        const vx_IrVar in = block->ins[i];
        fprintf(out, " %%%zu", in);
    }
    fputc('\n', out);

    for (size_t i = 0; i < block->ops_len; i ++) {
        vx_IrOp_dump(block->ops + i, out, indent + 1);
    }

    for (size_t i = 0; i < indent; i ++)
        fputs("  ", out);

    fputs("return", out);
    for (size_t i = 0; i < block->outs_len; i ++) {
        const vx_IrVar var = block->outs[i];
        fprintf(out, " %%%zu", var);
    }
    fputc('\n', out);
}
