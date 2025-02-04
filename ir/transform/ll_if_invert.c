#include "../passes.h"

// on the low level, it is better for if statements with only one case to only have an else body, not an if body.

void vx_IrBlock_ll_if_invert(vx_CU* cu, vx_IrBlock *block)
{
	for (vx_IrOp* op = block->first; op; op = op->next)
	{
		if (op->id != VX_IR_OP_IF)
			continue;

		vx_IrBlock *then = NULL; {
			vx_IrValue *val = vx_IrOp_param(op, VX_IR_NAME_COND_THEN);
			if (val)
				then = val->block;
		}

		vx_IrBlock *els = NULL; {
			vx_IrValue *val = vx_IrOp_param(op, VX_IR_NAME_COND_ELSE);
			if (val)
				els = val->block;
		}

		vx_IrValue cond_val = *vx_IrOp_param(op, VX_IR_NAME_COND);
		assert(cond_val.type != VX_IR_VAL_BLOCK);

		if (then && !els)
		{
			// then => else and invert condition

			vx_IrType* condType = vx_IrValue_type(cu, vx_IrBlock_root(block), cond_val);
			assert(condType);
			vx_IrOp* inv = vx_IrBlock_insertOpCreateAfter(block, vx_IrOp_predecessor(op), VX_IR_OP_NOT);
			vx_IrVar invVar = vx_IrBlock_newVar(block, inv);
			vx_IrOp_addOut(inv, invVar, condType);
			vx_IrOp_addParam_s(inv, VX_IR_NAME_VALUE, cond_val);

			*vx_IrOp_param(op, VX_IR_NAME_COND) = VX_IR_VALUE_VAR(invVar);

			vx_IrOp_removeParam(op, VX_IR_NAME_COND_THEN);
			vx_IrOp_addParam_s(op, VX_IR_NAME_COND_ELSE, VX_IR_VALUE_BLK(then));
		}
	}
}
