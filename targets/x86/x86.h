#include "../../ir/ir.h"
#include "../../ir/passes.h"

void vx_llir_x86_conditionals(vx_CU* cu, vx_IrBlock* block);
void vx_cg_x86stupid_gen(vx_CU* cu, vx_IrBlock* block, FILE* out);

void vx_llir_x86(vx_CU* cu, vx_IrBlock* block);

typedef struct {
	bool no_cg;
} vx_IrOp_x86;

typedef struct {

} vx_IrBlock_x86;

vx_IrOp_x86* vx_IrOp_x86_get(vx_IrOp* op);
vx_IrBlock_x86* vx_IrBlock_x86_get(vx_IrBlock* block);
