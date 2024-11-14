#include "../passes.h"

void vx_tra_move_rets_to_arg(vx_CU* cu, vx_IrBlock* block)
{
    assert(block->is_root);

    vx_IrType* ptrTy = vx_ptrType(block).ptr;

    vx_IrTypedVar* newArgs = malloc(sizeof(vx_IrTypedVar) * block->outs_len);
    size_t newArgs_count = 0;

    vx_IrVar* newRets = malloc(sizeof(vx_IrVar) * block->outs_len);
    size_t newRets_count = 0;

    for (size_t i = 0; i < block->outs_len; i ++) 
    {
        vx_IrVar var = block->outs[i];

        if (!cu->info.move_ret_to_arg(cu, block, i)) {
            newRets[newRets_count ++] = var;
            continue;
        }

        vx_IrOp* mov = vx_IrBlock_addOpBuilding(block);
        vx_IrOp_init(mov, VX_IR_OP_STORE, block);

        vx_IrVar resVar = vx_IrBlock_newVar(block, mov);
        newArgs[newArgs_count ++] = (vx_IrTypedVar) { .var = resVar, .type = ptrTy };

        vx_IrOp_addParam_s(mov, VX_IR_NAME_ADDR, VX_IR_VALUE_VAR(resVar));
        vx_IrOp_addParam_s(mov, VX_IR_NAME_VALUE, VX_IR_VALUE_VAR(var));
    }

    if (newArgs_count == 0)
        return;

    free(block->outs);
    block->outs = newRets;
    block->outs_len = newRets_count;

    // TODO: multiple insert function
    vx_IrTypedVar* oldIns = block->ins;
    block->ins = malloc(sizeof(vx_IrTypedVar) * (block->ins_len + newArgs_count));
    memcpy(block->ins, newArgs, sizeof(vx_IrTypedVar) * newArgs_count);
    memcpy(block->ins + newArgs_count, oldIns, sizeof(vx_IrTypedVar) * block->ins_len);
    block->ins_len += newArgs_count;
    free(oldIns);

    free(newArgs);
}
