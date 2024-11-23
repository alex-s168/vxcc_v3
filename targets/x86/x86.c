#include "../targets_internal.h"

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
	dest->cmov_opt = true; // TODO: target->flags.x86[vx_Target_X86_CMOV];
	dest->tailcall_opt = true;
	dest->ea_opt = true;
	dest->need_move_ret_to_arg = x86_need_move_ret_to_arg;
	x86_intlikeptr(dest);
}
