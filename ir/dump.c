#include "ir.h"

const char *vx_IrName_str[VX_IR_NAME__LAST] = {
    [VX_IR_NAME_OPERAND_A] = "a",
    [VX_IR_NAME_OPERAND_B] = "b",

    [VX_IR_NAME_BLOCK] = "block",
    [VX_IR_NAME_VALUE] = "val",
    [VX_IR_NAME_ADDR] = "addr",
    [VX_IR_NAME_COND] = "cond",
    [VX_IR_NAME_VAR] = "var",
    [VX_IR_NAME_SIZE] = "size",

    [VX_IR_NAME_COND_THEN] = "then",
    [VX_IR_NAME_COND_ELSE] = "else",

    [VX_IR_NAME_LOOP_DO] = "do",
    [VX_IR_NAME_LOOP_START] = "start",
    [VX_IR_NAME_LOOP_ENDEX] = "endex",
    [VX_IR_NAME_LOOP_STRIDE] = "stride",

    [VX_IR_NAME_IDX] = "idx",
    [VX_IR_NAME_ELSIZE] = "elsize",
    [VX_IR_NAME_OFF] = "off",

    [VX_IR_NAME_ID] = "id",
    [VX_IR_NAME_TYPE] = "type",
    [VX_IR_NAME_STRUCT] = "struct",
};

void vx_IrValue_dump(vx_IrValue value, FILE *out, const size_t indent) {
    assert(out);

    switch (value.type) {
        case VX_IR_VAL_BLOCKREF: {
            fprintf(out, "#%s", value.block->name);
        }
        break;

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

		case VX_IR_VAL_X86_CC: {
			fprintf(out, "%s", value.x86_cc);
		}
		break;

		case VX_IR_VAL_SYMREF: {
			fprintf(out, "%s", value.symref);
		}
		break;

        case VX_IR_VAL_BLOCK: {
            const vx_IrBlock *block = value.block;

            fputc('(', out);
            for (size_t i = 0; i < block->ins_len; i ++) {
                if (i > 0)
                    fputc(',', out);
                const vx_IrVar in = block->ins[i].var;
                fprintf(out, "%%%zu", in);
            }
            fputs("){\n", out);

            for (vx_IrOp *op = block->first; op; op = op->next) {
                vx_IrOp_dump(op, out, indent + 1);
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

	default:
	    assert(false);
	break;
    }
}

void vx_IrOp_dump(const vx_IrOp *op, FILE *out, size_t indent) {
    assert(out);
    assert(op);

    for (size_t j = 0; j < indent; j ++)
        fputs("  ", out);

    for (size_t j = 0; j < op->outs_len; j ++) {
        if (j > 0)
            fputc(',', out);
        const vx_IrTypedVar var = op->outs[j];
        fprintf(out, "%s %%%zu", var.type ? var.type->debugName : "NULLTYPE", var.var);
    }

    if (op->outs_len > 0)
        fputs(" = ", out);

    fprintf(out, "%s ", vx_IrOpType__entries[op->id].debug.a);

    for (size_t i = 0; i < op->args_len; i ++) {
        const vx_IrValue val = op->args[i];
        fprintf(out, "state%zu=", i);
        vx_IrValue_dump(val, out, indent);
        fputc(' ', out);
    }

    for (size_t j = 0; j < op->params_len; j ++) {
        if (j > 0)
            fputc(' ', out);
        const vx_IrNamedValue param = op->params[j];
        fprintf(out, "%s=", vx_IrName_str[param.name]);
        vx_IrValue_dump(param.val, out, indent);
    }

    fputs(";\n", out);
}

void vx_IrBlock_dump(vx_IrBlock *block, FILE *out, const size_t indent) {
    assert(out);

    for (size_t i = 0; i < indent; i ++)
        fputs("  ", out);

    fprintf(out, "BLOCK(%s)", block->name);
    for (size_t i = 0; i < block->ins_len; i ++) {
        if (i > 0)
            fputc(',', out);
        vx_IrTypedVar in = block->ins[i];
        fprintf(out, " %s %%%zu", in.type->debugName, in.var);
    }
    fputc('\n', out);

    for (vx_IrOp *op = block->first; op; op = op->next) {
        vx_IrOp_dump(op, out, indent + 1);
    }

    for (size_t i = 0; i < indent; i ++)
        fputs("  ", out);

    fputs("end", out);
    for (size_t i = 0; i < block->outs_len; i ++) {
        const vx_IrVar var = block->outs[i];
        fprintf(out, " %%%zu", var);
    }
    fputc('\n', out);
}
