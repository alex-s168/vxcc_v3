#include "x86.h"
#include "../../ir/passes.h"

void vx_llir_x86_conditionals(vx_CU* cu, vx_IrBlock* block)
{
	for (vx_IrOp* op = block->first; op; op = op->next)
	{
		char const* cc;
		switch (op->id) {
		case VX_IR_OP_UGT:  cc = "a"; break;
		case VX_IR_OP_ULT:  cc = "b"; break;
		case VX_IR_OP_UGTE: cc = "ae"; break;
		case VX_IR_OP_ULTE: cc = "be"; break;
		case VX_IR_OP_SGT:  cc = "g"; break;
		case VX_IR_OP_SLT:  cc = "l"; break;
		case VX_IR_OP_SGTE: cc = "ge"; break;
		case VX_IR_OP_SLTE: cc = "le"; break;
		case VX_IR_OP_EQ:   cc = "e"; break;
		case VX_IR_OP_NEQ:  cc = "ne"; break;
		case VX_IR_OP_BITEXTRACT: cc = "nz"; break;
		default:
			continue;
		}

		op->id = op->id == VX_IR_OP_BITEXTRACT
			? VX_IR_OP_X86_BITTEST 
			: VX_IR_OP_X86_CMP
			;

		vx_IrTypedVar out = op->outs[0];
		vx_IrOp_removeOutAt(op, 0);

		for (vx_IrOp* next = op->next; next; next = next->next)
		{
			bool breakLater = 
				vx_IrOpType__entries[next->id].x86_affect_flags.set && vx_IrOpType__entries[next->id].x86_affect_flags.a;

			bool breakNow = false;
			vx_IrOpType to;
			switch (next->id)
			{
				case VX_IR_OP_CMOV:
					to = VX_IR_OP_X86_CMOV;
					break;

				case VX_IR_OP_COND:
					to = VX_IR_OP_X86_JMPCC;
					break;

				default:
					if (breakLater) breakNow = true;
					else continue;
					break;
			}
			if (breakNow) break;

			vx_IrValue* next_cond = vx_IrOp_param(next, VX_IR_NAME_COND);

			if (next_cond->var == out.var) {
				*next_cond = VX_IR_VALUE_X86_CC(cc);
				next->id = to;
			}

			if (breakLater) break;
		}

		vx_IrOp* setcc = vx_IrBlock_insertOpCreateAfter(block, op, VX_IR_OP_X86_SETCC);

		vx_IrOp_addOut(setcc, out.var, out.type);
		vx_IrOp_addParam_s(setcc, VX_IR_NAME_COND, VX_IR_VALUE_X86_CC(cc));
	}

	for (vx_IrOp* op = block->first; op; op = op->next) {
		switch (op->id) {
			case VX_IR_OP_COND: op->id = VX_IR_OP_X86_JMPCC; break;
			case VX_IR_OP_CMOV: op->id = VX_IR_OP_X86_CMOV; break;
			default: continue;
		}

		vx_IrOp* test = vx_IrBlock_insertOpCreateAfter(block, vx_IrOp_predecessor(op), VX_IR_OP_X86_CMP);
		vx_IrOp_addParam_s(test, VX_IR_NAME_OPERAND_A, *vx_IrOp_param(op, VX_IR_NAME_COND));
		vx_IrOp_addParam_s(test, VX_IR_NAME_OPERAND_B, VX_IR_VALUE_IMM_INT(0));

		vx_IrOp_addParam_s(op, VX_IR_NAME_COND, VX_IR_VALUE_X86_CC("ne"));
	}
}
