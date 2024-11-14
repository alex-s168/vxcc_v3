#include "common.h"

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

static bool x86_need_move_ret_to_arg(vx_CU* cu, vx_IrBlock* block, size_t ret_id)
{
    // TODO
    return ret_id >= 2;
}

static bool etca_need_move_ret_to_arg(vx_CU*, vx_IrBlock* block, size_t ret_id)
{
    // TODO
    return ret_id >= 1;
}

void vx_Target_info(vx_TargetInfo* dest, vx_Target const* target)
{
    memset(dest, 0, sizeof(vx_TargetInfo));
    switch (target->arch)
    {
        case vx_TargetArch_X86_64:
            {
                dest->cmov_opt = true; // TODO: target->flags.x86[vx_Target_X86_CMOV];
                dest->tailcall_opt = true;
                dest->ea_opt = true;
                dest->need_move_ret_to_arg = x86_need_move_ret_to_arg;
	    } break;

        case vx_TargetArch_ETCA:
            {
                dest->cmov_opt = target->flags.etca[vx_Target_ETCA_condExec];
                dest->tailcall_opt = true;
                dest->need_move_ret_to_arg = etca_need_move_ret_to_arg;
	    } break;

        // add target
    }
}


