#include "../llir.h"

// merge lifetimes based on when and their type (are compatible)
// can not merge two lifetimes where one of them was ever place() d because we have to keep the pointer till the end of the fn

// if start of lifetime A == end of lifetime B, we can still merge 

// block needs to be 100% flat, decl of vars must be known, decl won't be known after this fn anymore

// TODO: figure out why it doesn't apply for overlapping*? lifetimes

void vx_IrBlock_ll_share_slots(vx_IrBlock *block) {
    for (vx_IrVar var = 0; var < block->as_root.vars_len; var ++) {
        if (block->as_root.vars[var].decl_parent == NULL)
            continue;

        // TODO: only search in lifetime: IrView of lifetime
        vx_IrView view = vx_IrView_of_all(block);
        bool get_rid = false;
        while (vx_IrView_find(&view, VX_IR_OP_PLACE)) {
            const vx_IrOp *op = vx_IrView_take(view);

            for (size_t i = 0; i < op->params_len; i ++) {
                if (op->params[i].val.id == VX_IR_VAL_VAR && op->params[i].val.var == var) {
                    get_rid = true;
                    break;
                }
            }

            if (get_rid)
                break;

            view = vx_IrView_drop(view, 1);
        }

        if (get_rid) {
            block->as_root.vars[var].ll_lifetime = (lifetime){0};
        }
    }

    for (vx_IrVar var = 0; var < block->as_root.vars_len; var ++) {
        lifetime *lt = &block->as_root.vars[var].ll_lifetime;

        if (lt->first == NULL)
            continue;
        
        vx_IrType *type = block->as_root.vars[var].ll_type;

        for (vx_IrVar var2 = 0; var < block->as_root.vars_len; var ++) {
            if (var == var2)
                continue;

            lifetime *lt2 = &block->as_root.vars[var].ll_lifetime;
            if (lt2->first == NULL)
                continue; 

            vx_IrType *type2 = block->as_root.vars[var].ll_type;

            if (!vx_IrType_compatible(type2, type))
                continue;

            lifetime cmp_low = *lt;
            lifetime cmp_high = *lt2;
            if (cmp_low.first > cmp_high.first) {
                cmp_low = *lt2;
                cmp_high = *lt;
            }

            if (cmp_low.last > cmp_high.first)
                continue;

            // it's okay if end0 = start1 

            vx_IrView view = vx_IrView_of_all(block);
            vx_IrView_rename_var(view, block, var, var2);

            break;
        }
    }
}
