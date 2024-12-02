#include "../targets_internal.h"
#include "x86.h"
#include "../../ir/passes.h"

void vx_llir_x86(vx_CU* cu, vx_IrBlock* block) {
	vx_llir_x86_conditionals(cu, block);	
	vx_opt_vars(cu, block);
}

typedef struct {
	vx_IrType* ptrty_cache;
} x86;
intlikeptr(x86, x86*,->ptrty_cache, 8, 4);

static bool x86_need_move_ret_to_arg(vx_CU* cu, vx_IrBlock* block, size_t ret_id)
{
    // TODO
    return ret_id >= 2;
}

void vx_Target_X86__info(vx_TargetInfo* dest, vx_Target const* target)
{
	set_tg(x86);
	dest->cmov_opt = target->flags.x86[vx_Target_X86_CMOV];
	dest->tailcall_opt = true;
	dest->ea_opt = true;
	dest->need_move_ret_to_arg = x86_need_move_ret_to_arg;
	x86_intlikeptr(dest);
}

vx_IrOp_x86* vx_IrOp_x86_get(vx_IrOp* op) {
	if (op->backend == NULL) {
		op->backend = fastalloc(sizeof(vx_IrOp_x86));
		memset(op->backend, 0, sizeof(vx_IrOp_x86));
	}
	return (vx_IrOp_x86*) op->backend;
}

vx_IrBlock_x86* vx_IrBlock_x86_get(vx_IrBlock* block) {
	if (block->backend == NULL) {
		block->backend = fastalloc(sizeof(vx_IrBlock_x86));
		memset(block->backend, 0, sizeof(vx_IrBlock_x86));
	}
	return (vx_IrBlock_x86*) block->backend;
}
