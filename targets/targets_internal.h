#include "../ir/ir.h"

/** these functions are used by targets where a pointer is a integer */
#define intlikeptr(name, tgty, ptrtotypecache, ptrwbytes, ptralignbytes) \
static vx_IrType* name##_get_ptr_ty(vx_CU* cu, vx_IrBlock* root) { \
	if (  ( ((tgty) cu->info.tg) ptrtotypecache) != NULL) return ( ((tgty) cu->info.tg) ptrtotypecache); \
	vx_IrType* ptr = vx_IrType_heap(); \
	( ((tgty) cu->info.tg) ptrtotypecache) = ptr; \
	ptr->debugName = "ptr"; \
	ptr->kind = VX_IR_TYPE_KIND_BASE; \
	ptr->base = (vx_IrTypeBase) { \
		.size = (ptrwbytes), .align = (ptralignbytes), \
		.isfloat = false, \
	}; \
	return ptr;\
} \
static vx_IrValue name##_get_null_ptr(vx_CU* cu, vx_IrBlock* dest) { \
	return VX_IR_VALUE_IMM_INT(0); \
} \
static void name##_lower_ptr_math(vx_CU* cu, vx_IrOp* op) { /* no-op */ } \
static vx_IrValue name##_cast_ptr_to_human(vx_CU* cu, vx_IrBlock* block, vx_IrVar ptr) { \
	return VX_IR_VALUE_VAR(ptr); \
} \
static void name##_intlikeptr(vx_TargetInfo* dest) { \
	dest->get_ptr_ty = name##_get_ptr_ty; \
	dest->get_null_ptr = name##_get_null_ptr; \
	dest->lower_ptr_math = name##_lower_ptr_math; \
	dest->cast_ptr_to_human = name##_cast_ptr_to_human; \
}

#define set_tg(ty) \
	dest->tg = fastalloc(sizeof(ty)); \
	memset(dest->tg, 0, sizeof(ty));

void vx_Target_X86__info(vx_TargetInfo* dest, vx_Target const* target);
void vx_Target_ETCA__info(vx_TargetInfo* dest, vx_Target const* target);
