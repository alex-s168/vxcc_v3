#include "../ir/ir.h"

int vx_Target_parse(vx_Target* dest, const char * str)
{
    bool found = false;
    size_t strLen = strlen(str);
for (vx_TargetArch t = 0; t < vx_TargetArch__len; t ++)
    {
        const char * name = vx_TargetArch__entries[t].name.a;
        size_t nameLen = strlen(name);
        if (strLen < nameLen) break;
        if (!memcmp(str, name, nameLen) && (str[nameLen] == ':' || str[nameLen] == '\0'))
        {
            found = true;
            dest->arch = t;
            str += nameLen;
            if (str[0] == ':') str ++;
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "Could not find target arch\n");
        return 1;
    }

    switch (dest->arch)
    {
        case vx_TargetArch_X86_64:
            vx_Target_X86__parseAdditionalFlags(&dest->flags.x86, str);
            break;

        case vx_TargetArch_ETCA:
            vx_Target_ETCA__parseAdditionalFlags(&dest->flags.etca, str);
            break;

        // add target
    }

    return 0;
}

/** these functions are used by targets where a pointer is a integer */
#define intlikeptr(name, tgty, ptrtotypecache, ptrwbytes, ptralignbytes) \
static vx_IrType* name##_get_ptr_ty(vx_CU* cu, vx_IrBlock* root) { \
	if (  ( ((tgty) cu->info.tg) ptrtotypecache) != NULL) return ( ((tgty) cu->info.tg) ptrtotypecache); \
	vx_IrType* ptr = vx_IrType_heap(); \
	( ((tgty) cu->info.tg) ptrtotypecache) = ptr; \
	ptr->debugName = "ptr"; \
	ptr->kind = VX_IR_TYPE_KIND_BASE; \
	ptr->base = (vx_IrTypeBase) { \
		.sizeless = false, .size = (ptrwbytes), .align = (ptralignbytes), \
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

typedef struct {
	vx_IrType* ptrty_cache;
} x86;
intlikeptr(x86, x86*,->ptrty_cache, 8, 4);

typedef struct {
	vx_IrType* ptrty_cache;
} etca;
intlikeptr(etca, etca*,->ptrty_cache, 4, 2);

static bool x86_need_move_ret_to_arg(vx_CU* cu, vx_IrBlock* block, size_t ret_id)
{
    // TODO
    return ret_id >= 2;
}

static bool etca_need_move_ret_to_arg(vx_CU* cu, vx_IrBlock* block, size_t ret_id)
{
    // TODO
    return ret_id >= 1;
}

void vx_Target_info(vx_TargetInfo* dest, vx_Target const* target)
{
	#define set_tg(ty) \
		dest->tg = fastalloc(sizeof(ty)); \
		memset(dest->tg, 0, sizeof(ty));

    memset(dest, 0, sizeof(vx_TargetInfo));
    switch (target->arch)
    {
        case vx_TargetArch_X86_64:
            {
				set_tg(x86);
                dest->cmov_opt = true; // TODO: target->flags.x86[vx_Target_X86_CMOV];
                dest->tailcall_opt = true;
                dest->ea_opt = true;
                dest->need_move_ret_to_arg = x86_need_move_ret_to_arg;
				x86_intlikeptr(dest);
	    } break;

        case vx_TargetArch_ETCA:
            {
				set_tg(etca);
                dest->cmov_opt = target->flags.etca[vx_Target_ETCA_condExec];
                dest->tailcall_opt = true;
                dest->need_move_ret_to_arg = etca_need_move_ret_to_arg;
				etca_intlikeptr(dest);
	    } break;

        // add target
    }
}


