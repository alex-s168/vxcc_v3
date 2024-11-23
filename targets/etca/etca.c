#include "../targets_internal.h"

typedef struct {
	vx_IrType* ptrty_cache;
} etca;
intlikeptr(etca, etca*,->ptrty_cache, 4, 2);

static bool etca_need_move_ret_to_arg(vx_CU* cu, vx_IrBlock* block, size_t ret_id)
{
    // TODO
    return ret_id >= 1;
}

void vx_Target_X86__info(vx_TargetInfo* dest, vx_Target const* target)
{
	set_tg(etca);
	dest->cmov_opt = target->flags.etca[vx_Target_ETCA_condExec];
	dest->tailcall_opt = true;
	dest->need_move_ret_to_arg = etca_need_move_ret_to_arg;
	etca_intlikeptr(dest);
}
