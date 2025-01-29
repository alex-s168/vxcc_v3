#include "../../ir/ir.h"
#include "elf.h"
#include <stdatomic.h>

// https://five-embeddev.com/riscv-user-isa-manual/Priv-v1.12/rv32.html
// https://msyksphinz-self.github.io/riscv-isadoc/html/rvi.html

// 7 bits
typedef enum {
    // Load instructions
    OPCODE_LOAD = 0x03,

    // Store instructions
    OPCODE_STORE = 0x23,

    // Immediate arithmetic/logic instructions
    OPCODE_OP_IMM = 0x13,

    // Register-register arithmetic/logic instructions
    OPCODE_OP = 0x33,

    // Branch instructions
    OPCODE_BRANCH = 0x63,

    // Jump and link register
    OPCODE_JALR = 0x67,

    // Jump and link
    OPCODE_JAL = 0x6F,

    // Upper immediate instructions
    OPCODE_LUI = 0x37,
    OPCODE_AUIPC = 0x17,

    // System instructions
    OPCODE_SYSTEM = 0x73,

    // Fence instructions
    OPCODE_FENCE = 0x0F,

    // Custom instructions (reserved for vendor-specific extensions)
    OPCODE_CUSTOM = 0x0B,

    // Atomic memory operations (A extension)
    OPCODE_AMO = 0x2F,
} opcode_t;

typedef enum {
    // funct3 values for OPCODE_LOAD (0x03)
    FUNCT3_LB  = 0x0,  // Load byte
    FUNCT3_LH  = 0x1,  // Load halfword
    FUNCT3_LW  = 0x2,  // Load word
    FUNCT3_LBU = 0x4,  // Load byte unsigned
    FUNCT3_LHU = 0x5,  // Load halfword unsigned

    // funct3 values for OPCODE_STORE (0x23)
    FUNCT3_SB  = 0x0,  // Store byte
    FUNCT3_SH  = 0x1,  // Store halfword
    FUNCT3_SW  = 0x2,  // Store word

    // funct3 values for OPCODE_OP_IMM (0x13) and OPCODE_OP (0x33)
    FUNCT3_ADD_SUB = 0x0,  // ADD/SUB
    FUNCT3_SLL     = 0x1,  // Shift left logical
    FUNCT3_SLT     = 0x2,  // Set less than
    FUNCT3_SLTU    = 0x3,  // Set less than unsigned
    FUNCT3_XOR     = 0x4,  // XOR
    FUNCT3_SRL_SRA = 0x5,  // Shift right logical/arithmetic
    FUNCT3_OR      = 0x6,  // OR
    FUNCT3_AND     = 0x7,  // AND

    // funct3 values for OPCODE_BRANCH (0x63)
    FUNCT3_BEQ  = 0x0,  // Branch if equal
    FUNCT3_BNE  = 0x1,  // Branch if not equal
    FUNCT3_BLT  = 0x4,  // Branch if less than
    FUNCT3_BGE  = 0x5,  // Branch if greater than or equal
    FUNCT3_BLTU = 0x6,  // Branch if less than unsigned
    FUNCT3_BGEU = 0x7,  // Branch if greater than or equal unsigned

    // funct3 values for OPCODE_JALR (0x67)
    FUNCT3_JALR = 0x0,  // Jump and link register

    // funct3 values for OPCODE_SYSTEM (0x73)
    FUNCT3_ECALL_EBREAK = 0x0,  // ECALL/EBREAK
    FUNCT3_CSRRW        = 0x1,  // CSR read/write
    FUNCT3_CSRRS        = 0x2,  // CSR read and set
    FUNCT3_CSRRC        = 0x3,  // CSR read and clear
    FUNCT3_CSRRWI       = 0x5,  // CSR read/write immediate
    FUNCT3_CSRRSI       = 0x6,  // CSR read and set immediate
    FUNCT3_CSRRCI       = 0x7,  // CSR read and clear immediate

    // funct3 values for OPCODE_FENCE (0x0F)
    FUNCT3_FENCE   = 0x0,  // FENCE
    FUNCT3_FENCE_I = 0x1,  // FENCE.I
} funct3_t;

typedef enum {
    // funct7 values for OPCODE_OP (0x33)
    FUNCT7_ADD = 0x00,  // ADD
    FUNCT7_SUB = 0x20,  // SUB
    FUNCT7_SLL = 0x00,  // Shift left logical
    FUNCT7_SLT = 0x00,  // Set less than
    FUNCT7_SLTU = 0x00, // Set less than unsigned
    FUNCT7_XOR = 0x00,  // XOR
    FUNCT7_SRL = 0x00,  // Shift right logical
    FUNCT7_SRA = 0x20,  // Shift right arithmetic
    FUNCT7_OR  = 0x00,  // OR
    FUNCT7_AND = 0x00,  // AND

    // funct7 values for OPCODE_OP_IMM (0x13)
    FUNCT7_SLLI = 0x00, // Shift left logical immediate
    FUNCT7_SRLI = 0x00, // Shift right logical immediate
    FUNCT7_SRAI = 0x20, // Shift right arithmetic immediate

    // funct7 values for OPCODE_SYSTEM (0x73)
    FUNCT7_ECALL  = 0x00,  // ECALL
    FUNCT7_EBREAK = 0x01,  // EBREAK
} funct7_t;

// 5 bits
typedef enum {
	XZERO = 0,
	X1,X2,X3,X4,X5,X6,X7,
	X8,X9,X10,X11,X12,X13,X14,X15,
	X16,X17,X18,X19,X20,X21,X22,X23,
	X24,X25,X26,X27,X28,X29,X30,X31
} reg;

// reg reg => reg
static void gen_r(vx_ObjWriter out, opcode_t op, reg src0, reg src1, reg dest, funct7_t funct7, funct3_t funct3) {
	uint32_t res = funct7;
	res <<= 5; res |= src1;
	res <<= 5; res |= src0;
	res <<= 3; res |= funct3;
	res <<= 5; res |= dest;
	res <<= 7; res |= op;

	out.writeBytes(out.ud, &res, sizeof(res));
}

// reg imm => reg
static void gen_i(vx_ObjWriter out, opcode_t op, reg src0, uint32_t imm12, reg dest, funct3_t funct3) {
	uint32_t res = imm12;
	res <<= 5; res |= src0;
	res <<= 3; res |= funct3;
	res <<= 5; res |= dest;
	res <<= 7; res |= op;

	out.writeBytes(out.ud, &res, sizeof(res));
}

// reg reg imm => void
static void gen_s(vx_ObjWriter out, opcode_t op, reg src0, reg src1, uint32_t imm12, funct3_t funct3) {
	uint32_t res = imm12 >> 5;
	res <<= 5; res |= src1;
	res <<= 5; res |= src0;
	res <<= 3; res |= funct3;
	res <<= 5; res |= imm12 & 0b11111;
	res <<= 7; res |= op;

	out.writeBytes(out.ud, &res, sizeof(res));
}

// reg reg imm => void
static void gen_b(vx_ObjWriter out, opcode_t op, reg src0, reg src1, uint32_t imm13, funct3_t funct3) {
	assert(((imm13 & 1) == 0) && "b mode is used for branch instructions. why would you have a jump to a unaligned address????");
	uint32_t res = imm13 >> 12;
	res <<= 6; res |= (imm13 >> 5) & 0b111111;
	res <<= 5; res |= src1;
	res <<= 5; res |= src0;
	res <<= 3; res |= funct3;
	res <<= 4; res |= (imm13 >> 1) & 0b1111;
	res <<= 1; res |= (imm13 >> 11) & 1;
	res <<= 7; res |= op;

	out.writeBytes(out.ud, &res, sizeof(res));
}

// imm => dest
static void gen_u(vx_ObjWriter out, opcode_t op, uint32_t imm32, reg dest) {
	assert(((imm32 & 0b111111111111) == 0) && "first 12 bits in u type immediate need to be zero");
	uint32_t res = imm32 >> 12;
	res <<= 5; res |= dest;
	res <<= 7; res |= op;

	out.writeBytes(out.ud, &res, sizeof(res));
}

// imm => dest
static void gen_j(vx_ObjWriter out, opcode_t op, uint32_t imm21, reg dest) {
	assert(((imm21 & 1) == 0) && "j mode is used for branch instructions. why would you have a jump to a unaligned address????");
	uint32_t res = imm21 >> 20;
	res <<= 10; res |= (imm21 >> 1) & 0b1111111111;
	res <<= 1; res |= (imm21 >> 11) & 1;
	res <<= 8; res |= (imm21 >> 12) & 0b11111111;
	res <<= 5; res |= dest;
	res <<= 7; res |= op;

	out.writeBytes(out.ud, &res, sizeof(res));
}

// calling conv: https://riscv.org/wp-content/uploads/2024/12/riscv-calling.pdf

// do NOT move to Ctx
static atomic_size_t next_local_label = 0;

typedef enum {
	LOC_NONE = 0,
	LOC_STACK,
	LOC_REG,
} LocKind;

typedef struct {
	LocKind kind;
	union {
		reg r;
		size_t stack;
	};
} Loc;

typedef struct {
	Loc location;
} CtxVar;

typedef struct {
	vx_CU* cu;
	CtxVar* vars;
	vx_ObjWriter out;
} Ctx;

void vx_cg_rv32_objgen(vx_ObjWriter out, vx_CU* cu, vx_IrBlock* block)
{
	Ctx ctx;
	ctx.vars = calloc(sizeof(CtxVar), block->ins_len);
	ctx.cu = cu;
	ctx.out = out;

	// TODO: allow weak decl
	out.putDeclNext(out.ud, block->name, true, false);

	bool is_leaf = vx_IrBlock_llIsLeaf(block);

	// not yet fully initialized
	bool need_stack_frame = !is_leaf || vx_IrBlock_anyPlaced(block);

	size_t varsHotFirstLen;
    vx_IrVar* varsHotFirst = vx_IrBlock_sortVarsHotFirst(block, &varsHotFirstLen);

	for (size_t i = 0; i < varsHotFirstLen; i ++)
	{
		vx_IrVar var = varsHotFirst[i];

		bool is_arg = false;
		for (size_t j = 0; j < block->ins_len; j ++) {
			if (block->ins[j].var == var) {
				is_arg = true;
				break;
			}
		}

		if (is_arg) {
			continue;
		}

		// TODO: alloc regs and maybe set need_stack_frame
	}

	free(varsHotFirst);
}

// TODO: elf relocs:
//	https://github.com/llvm/llvm-project/blob/5a34e6fdceac40da3312d96273e4b5d767f4a481/llvm/include/llvm/BinaryFormat/ELFRelocs/RISCV.def
// 	https://github.com/llvm/llvm-project/blob/main/llvm/lib/Target/RISCV/MCTargetDesc/RISCVFixupKinds.h
