#include "ir.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


bool vx_IrBlock_eval_var(vx_IrBlock *block, vx_IrVar var, vx_IrValue *dest) {
    vx_IrOp *decl = vx_IrBlock_root(block)->as_root.vars[var].decl;
    if (decl == NULL)
        return false;

    if (decl->id == VX_IR_OP_IMM) {
        const vx_IrValue *value = vx_IrOp_param(decl, VX_IR_NAME_VALUE);
        if (value == NULL)
            return false;

        if (value->type == VX_IR_VAL_VAR)
            return vx_IrBlock_eval_var(block, value->var, dest);

        *dest = *value;
        return true;
    }

    return false;
}

bool vx_Irblock_mightbe_var(vx_IrBlock *block, vx_IrVar var, vx_IrValue v) {
    vx_IrValue is;
    if (vx_IrBlock_eval_var(block, var, &is)) {
        return memcmp(&is, &v, sizeof(vx_IrValue)) == 0;
    }
    return true;
}

bool vx_Irblock_alwaysis_var(vx_IrBlock *block, vx_IrVar var, vx_IrValue v) {
    vx_IrValue is;
    if (!vx_IrBlock_eval_var(block, var, &is))
        return false;
    return memcmp(&is, &v, sizeof(vx_IrValue)) == 0;
}

void vx_Irblock_eval(vx_IrBlock *block, vx_IrValue *v) {
    if (v->type == VX_IR_VAL_VAR)
        while (vx_IrBlock_eval_var(block, v->var, v)) {}
}


struct IrStaticIncrement vx_IrOp_detect_static_increment(vx_IrOp *op) {
    if (op->id != VX_IR_OP_ADD && op->id != VX_IR_OP_SUB)
        return (struct IrStaticIncrement) { .detected = false };

    const vx_IrValue a = *vx_IrOp_param(op, VX_IR_NAME_OPERAND_A);
    const vx_IrValue b = *vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);

    if (a.type == VX_IR_VAL_VAR && b.type == VX_IR_VAL_IMM_INT) {
        long long by = b.imm_int;
        if (op->id == VX_IR_OP_SUB)
            by = -by;
        return (struct IrStaticIncrement) {
            .detected = false,
            .var = a.var,
            .by = by
        };
    }

    if (b.type == VX_IR_VAL_VAR && a.type == VX_IR_VAL_IMM_INT && op->id == VX_IR_OP_ADD) {
        return (struct IrStaticIncrement) {
            .detected = false,
            .var = b.var,
            .by = a.imm_int
        };
    }

    return (struct IrStaticIncrement) { .detected = false };
}
