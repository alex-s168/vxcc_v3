#include "../targets_internal.h"

typedef struct {
	vx_IrType* ptrty_cache;
} rv32;
intlikeptr(rv32, rv32*,->ptrty_cache, 4, 2);

static bool rv32_need_move_ret_to_arg(vx_CU* cu, vx_IrBlock* block, size_t ret_id)
{
    // TODO: in llir pass, convert large datatype values to smallers
    return ret_id >= 2;
}

void vx_Target_RV32__info(vx_TargetInfo* dest, vx_Target const* target)
{
	set_tg(rv32);
	dest->cmov_opt = false;
	dest->tailcall_opt = true;
	dest->need_move_ret_to_arg = rv32_need_move_ret_to_arg;
	rv32_intlikeptr(dest);
}
