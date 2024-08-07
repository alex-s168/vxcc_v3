#include "../llir.h"

// merge lifetimes based on when and their type (are compatible)
// can not merge two lifetimes where one of them was ever place() d because we have to keep the pointer till the end of the fn

// if start of lifetime A == end of lifetime B, we can still merge 

// block needs to be 100% flat, decl of vars must be known, decl won't be known after this fn anymore

// TODO: figure out why it doesn't apply for overlapping*? lifetimes

void vx_IrBlock_ll_share_slots(vx_IrBlock *block) {
    for (vx_IrVar var = 0; var < block->as_root.vars_len; var ++) {
        if (block->as_root.vars[var].decl == NULL)
            continue;

        // TODO (perf): only search in lifetime
        bool get_rid = false;
        for (vx_IrOp *op = block->first; op; op = op->next) {
            for (size_t i = 0; i < op->params_len; i ++) {
                if (op->params[i].val.id == VX_IR_VAL_VAR && op->params[i].val.var == var) {
                    get_rid = true;
                    break;
                }
            }

            if (get_rid)
                break;
        }

        if (get_rid) {
            block->as_root.vars[var].ll_lifetime = (lifetime){0};
        }
    }

    for (vx_IrVar var = 0; var < block->as_root.vars_len; var ++) {
        lifetime *lt = &block->as_root.vars[var].ll_lifetime;

        if (lt->first == NULL) {
            continue;
        }
        
        vx_IrType *type = block->as_root.vars[var].ll_type;
        assert(type != NULL);
        size_t typeSize = vx_IrType_size(type);

        for (vx_IrVar var2 = 0; var < block->as_root.vars_len; var ++) {
            if (var == var2)
                continue;

            lifetime *lt2 = &block->as_root.vars[var].ll_lifetime;
            if (lt2->first == NULL)
                continue; 

            vx_IrType *type2 = block->as_root.vars[var].ll_type;
            assert(type2 != NULL);
            size_t type2Size = vx_IrType_size(type2);

            if (typeSize != type2Size) { // dont use vx_IrType_compatible() here
                printf("skipping (type size %zu vs %zu)\n", typeSize, type2Size);
                continue;
            }

            lifetime cmp_low = *lt;
            lifetime cmp_high = *lt2;
            if (cmp_low.first > cmp_high.first) {
                cmp_low = *lt2;
                cmp_high = *lt;
            }

            if (vx_IrOp_after(cmp_low.last, cmp_high.first)) {
                continue;
            }

            // it's okay if end0 = start1 

            vx_IrBlock_rename_var(block, var, var2);

            break;
        }
    }
}
