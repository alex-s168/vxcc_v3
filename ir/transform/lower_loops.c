#include "../passes.h"

// while loop to infinite loop with conidtional break

void vx_IrBlock_llir_preLower_loops(vx_CU* cu, vx_IrBlock *block)
{
    for (vx_IrOp* op = block->first; op; op = op->next)
    {
        FOR_INPUTS(op, inp, {
            if (inp.type == VX_IR_VAL_BLOCK)
                vx_IrBlock_llir_preLower_loops(cu, inp.block);
        });

        if (op->id != VX_IR_OP_WHILE) continue;

        vx_IrBlock* cond = vx_IrOp_param(op, VX_IR_NAME_COND)->block;
        vx_IrBlock* body = vx_IrOp_param(op, VX_IR_NAME_LOOP_DO)->block;

        free(op->params);
        op->params = NULL;
        op->params_len = 0;

        vx_IrBlock* newBody = vx_IrBlock_initHeap(block, op);
        for (size_t i = 0; i < body->ins_len; i ++)
            vx_IrBlock_addIn(newBody, body->ins[i].var, body->ins[i].type);
        free(body->ins);
        body->ins = NULL;
        body->ins_len = 0;

        for (size_t i = 0; i < cond->ins_len; i ++) 
        {
            vx_IrOp* op = vx_IrBlock_addOpBuilding(newBody);
            vx_IrOp_init(op, VX_IR_OP_IMM, newBody);
            vx_IrOp_addOut(op, cond->ins[i].var, cond->ins[i].type);
            vx_IrOp_addParam_s(op, VX_IR_NAME_VALUE, VX_IR_VALUE_VAR(newBody->ins[i].var));
        }
        free(cond->ins);
        cond->ins = NULL;
        cond->ins_len = 0;

        vx_IrOp* ifOp = vx_IrBlock_addOpBuilding(newBody);
        vx_IrOp_init(ifOp, VX_IR_OP_IF, newBody);

        body->parent = ifOp->parent;
        vx_IrOp_addParam_s(ifOp, VX_IR_NAME_COND, VX_IR_VALUE_BLK(cond));

        body->parent = ifOp->parent;
        vx_IrOp_addParam_s(ifOp, VX_IR_NAME_COND_THEN, VX_IR_VALUE_BLK(body));

        {
            vx_IrBlock* els = vx_IrBlock_initHeap(ifOp->parent, ifOp);

            vx_IrOp* brk = vx_IrBlock_addOpBuilding(els);
            vx_IrOp_init(brk, VX_IR_OP_BREAK, els);

            for (size_t i = 0; i < newBody->ins_len; i ++)
            {
                vx_IrOp_addArg(brk, VX_IR_VALUE_VAR(newBody->ins[i].var));
                vx_IrBlock_addOut(els, newBody->ins[i].var);
            }

            vx_IrOp_addParam_s(ifOp, VX_IR_NAME_COND_ELSE, VX_IR_VALUE_BLK(els));
        }

        for (size_t i = 0; i < body->outs_len; i ++)
        {
            vx_IrVar newv = vx_IrBlock_newVar(newBody, ifOp);
            vx_IrOp_addOut(ifOp, newv, newBody->ins[i].type);
            vx_IrBlock_addOut(newBody, newv);
        }

        op->id = VX_IR_OP_INFINITE;
        vx_IrOp_addParam_s(op, VX_IR_NAME_LOOP_DO, VX_IR_VALUE_BLK(newBody));
    }
}
