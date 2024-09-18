#include "../llir.h"

static bool boolArrOverlap(bool* a, bool* b, size_t len)
{
    for (size_t i = 0; i < len; i ++)
    {
        if (a[i] && b[i])
        {
            return true;
        }
    }

    return false;
}

static size_t boolArrCount(bool* a, size_t len)
{
    size_t r = 0;
    for (size_t i = 0; i < len; i ++)
    {
        if (a[i])
        {
            r ++;
        }
    }
    return r;
}

static size_t boolArrFirst(bool* a, size_t len)
{
    for (size_t i = 0; i < len; i ++)
    {
        if (a[i])
        {
            return i;
        }
    }
    return len;
}

static size_t boolArrLast(bool* a, size_t len)
{
    size_t last = 0;
    for (size_t i = 0; i < len; i ++)
    {
        if (a[i])
        {
            last = i;
        }
    }
    return last;
}

static void boolArrOrAssign(bool* dest, bool* src, size_t len)
{
    for (size_t i = 0; i < len; i ++)
    {
        dest[i] = dest[i] || src[i];
    }
}

// merge lifetimes based on when and their type (are compatible)
// block needs to be 100% flat, decl of vars must be known, decl won't be known after this fn anymore

// TODO: second strategry that is configurable in config:
//   when the lifetimes do overlap, but one of them ends during the lt of the other, move other into dead lifetime

void vx_IrBlock_ll_share_slots(vx_IrBlock *block) {
    size_t blkInstLen = vx_IrBlock_countOps(block);
    size_t varsLen = block->as_root.vars_len;

    bool* renamed = fastalloc(sizeof(bool) * varsLen);
    memset(renamed, 0, sizeof(bool) * varsLen);

    for (vx_IrVar var = 0; var < block->as_root.vars_len; var ++) {
        if (vx_IrBlock_vardeclIsInIns(block, var))
            continue;

    recheck:
        if (renamed[var])
            continue;

        lifetime lt = block->as_root.vars[var].ll_lifetime;
        size_t heat1 = boolArrCount(lt.used_in_op, blkInstLen);

        vx_IrType* type1 = vx_IrBlock_typeofVar(block, var);
        if (type1 == NULL)
            continue;

        size_t typeSize1 = vx_IrType_size(type1);

        for (vx_IrVar var2 = 0; var2 < block->as_root.vars_len; var2 ++) {
            if (var == var2)
                continue;

            if (renamed[var2])
                continue;

            if (vx_IrBlock_vardeclIsInIns(block, var2))
                continue;

            lifetime lt2 = block->as_root.vars[var2].ll_lifetime;

            vx_IrType* type2 = vx_IrBlock_typeofVar(block, var2);
            if (type2 == NULL)
                continue;

            size_t typeSize2 = vx_IrType_size(type2); 

            if (typeSize1 != typeSize2)
                continue;

            bool canMerge = true;
            if (boolArrOverlap(lt.used_in_op, lt2.used_in_op, blkInstLen))
                canMerge = false;

            if (boolArrLast(lt.used_in_op, blkInstLen) == boolArrFirst(lt2.used_in_op, blkInstLen))
                canMerge = true;
            else if (boolArrFirst(lt.used_in_op, blkInstLen) == boolArrLast(lt2.used_in_op, blkInstLen))
                canMerge = true;

            if (!canMerge)
                continue;

            size_t heat2 = boolArrCount(lt.used_in_op, blkInstLen);

            if (heat1 > heat2)
            {
                vx_IrBlock_renameVar(block, var2, var);
                renamed[var2] = true;
                boolArrOrAssign(lt.used_in_op, lt2.used_in_op, blkInstLen);
                goto recheck;
            }
            else 
            {
                vx_IrBlock_renameVar(block, var, var2);
                renamed[var] = true;
                boolArrOrAssign(lt2.used_in_op, lt.used_in_op, blkInstLen);
                break; // don't want to continue finding matches for this var since we renamed it 
            }
        }
    }
}
