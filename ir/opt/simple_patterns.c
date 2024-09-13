#include "../opt.h"

void vx_opt_simple_patterns(vx_IrBlock* block)
{
    vx_IrBlock* root = vx_IrBlock_root(block);

    for (vx_IrOp* op = block->first; op; op = op->next) {
        vx_IrValue* operandA = vx_IrOp_param(op, VX_IR_NAME_OPERAND_A);
        vx_IrValue* operandB = vx_IrOp_param(op, VX_IR_NAME_OPERAND_B);

        // shift num 1 left 
        if (op->id == VX_IR_OP_SHL && operandA && operandA->type == VX_IR_VAL_IMM_INT && operandA->imm_int == 1) {
            op->id = VX_IR_OP_BITMASK;
            vx_IrOp_add_param_s(op, VX_IR_NAME_IDX, *operandB);
            vx_IrOp_remove_param(op, VX_IR_NAME_OPERAND_A);
            vx_IrOp_remove_param(op, VX_IR_NAME_OPERAND_B);
            continue;
        }

        // and with bitmask 
        if (op->id == VX_IR_OP_BITWISE_AND && operandA && operandB) {
            if (operandA->type == VX_IR_VAL_VAR) {
                vx_IrOp* decl = root->as_root.vars[operandA->var].decl;
                if (decl && decl->id == VX_IR_OP_BITMASK) {
                    vx_IrValue idx = *vx_IrOp_param(decl, VX_IR_NAME_IDX);
                    op->id = VX_IR_OP_BITEXTRACT;
                    vx_IrValue opB = *operandB;
                    vx_IrOp_remove_params(op);
                    vx_IrOp_add_param_s(op, VX_IR_NAME_IDX, idx);
                    vx_IrOp_add_param_s(op, VX_IR_NAME_VALUE, opB);
                    continue;
                }
            }

            if (operandB->type == VX_IR_VAL_VAR) {
                vx_IrOp* decl = root->as_root.vars[operandB->var].decl;
                if (decl && decl->id == VX_IR_OP_BITMASK) {
                    vx_IrValue idx = *vx_IrOp_param(decl, VX_IR_NAME_IDX);
                    op->id = VX_IR_OP_BITEXTRACT;
                    vx_IrValue opA = *operandA;
                    vx_IrOp_remove_params(op);
                    vx_IrOp_add_param_s(op, VX_IR_NAME_IDX, idx);
                    vx_IrOp_add_param_s(op, VX_IR_NAME_VALUE, opA);
                    continue;
                }
            }
        }

        // ptr + idx * mul
        if (op->id == VX_IR_OP_ADD && operandA && operandB) {
            if (operandA->type == VX_IR_VAL_VAR) {
                vx_IrOp* decl = root->as_root.vars[operandA->var].decl;
                if (decl && decl->id == VX_IR_OP_MUL) {
                    vx_IrValue mulA = *vx_IrOp_param(decl, VX_IR_NAME_OPERAND_A);
                    vx_IrValue mulB = *vx_IrOp_param(decl, VX_IR_NAME_OPERAND_B);
                    vx_IrValue base = *operandB;

                    op->id = VX_IR_OP_EA;
                    vx_IrOp_remove_params(op);
                    vx_IrOp_add_param_s(op, VX_IR_NAME_ADDR, base);
                    vx_IrOp_add_param_s(op, VX_IR_NAME_ELSIZE, mulA);
                    vx_IrOp_add_param_s(op, VX_IR_NAME_IDX, mulB);
                    continue;
                }
            }

            if (operandB->type == VX_IR_VAL_VAR) {
                vx_IrOp* decl = root->as_root.vars[operandB->var].decl;
                if (decl && decl->id == VX_IR_OP_MUL) {
                    vx_IrValue mulA = *vx_IrOp_param(decl, VX_IR_NAME_OPERAND_A);
                    vx_IrValue mulB = *vx_IrOp_param(decl, VX_IR_NAME_OPERAND_B);
                    vx_IrValue base = *operandA;

                    op->id = VX_IR_OP_EA;
                    vx_IrOp_remove_params(op);
                    vx_IrOp_add_param_s(op, VX_IR_NAME_ADDR, base);
                    vx_IrOp_add_param_s(op, VX_IR_NAME_ELSIZE, mulA);
                    vx_IrOp_add_param_s(op, VX_IR_NAME_IDX, mulB);
                    continue;
                }
            }
        }

        // load at ea 
        if (op->id == VX_IR_OP_LOAD) {
            vx_IrValue addr = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
            if (addr.type == VX_IR_VAL_VAR) {
                vx_IrOp* decl = root->as_root.vars[addr.var].decl;
                if (decl->id == VX_IR_OP_EA) {
                    op->id = VX_IR_OP_LOAD_EA;
                    vx_IrOp_remove_param(op, VX_IR_NAME_ADDR);
                    vx_IrOp_steal_param(op, decl, VX_IR_NAME_ADDR);
                    vx_IrOp_steal_param(op, decl, VX_IR_NAME_ELSIZE);
                    vx_IrOp_steal_param(op, decl, VX_IR_NAME_IDX);
                    continue;
                }
            }
        }

        // store at ea 
        if (op->id == VX_IR_OP_STORE) {
            vx_IrValue addr = *vx_IrOp_param(op, VX_IR_NAME_ADDR);
            if (addr.type == VX_IR_VAL_VAR) {
                vx_IrOp* decl = root->as_root.vars[addr.var].decl;
                if (decl->id == VX_IR_OP_EA) {
                    op->id = VX_IR_OP_STORE_EA;
                    vx_IrOp_remove_param(op, VX_IR_NAME_ADDR);
                    vx_IrOp_steal_param(op, decl, VX_IR_NAME_ADDR);
                    vx_IrOp_steal_param(op, decl, VX_IR_NAME_ELSIZE);
                    vx_IrOp_steal_param(op, decl, VX_IR_NAME_IDX);
                    continue;
                }
            }
        }
    }
}
