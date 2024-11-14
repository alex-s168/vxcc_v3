#include <stdbool.h>
#include <stdint.h>

#include "../ir/ir.h"
#include "../allib/germanstr/germanstr.h"


// serializable by memcpy
typedef uint8_t OperandType;
#define OPERAND_TYPE_PLACEHOLDER ((OperandType) 0)
#define OPERAND_TYPE_IMM_INT     ((OperandType) 1)
#define OPERAND_TYPE_IMM_FLT     ((OperandType) 2)

// serializable by memcpy
typedef struct {
    vx_IrName name;
    OperandType type;
    union {
        uint32_t placeholder;
        long long int imm_int;
        double imm_flt;
    } v;
} __attribute__((packed)) CompOperand;

typedef struct {
    bool is_any;

    union {
        struct {

        } any;

        struct {
            struct {
                uint32_t * items;
                uint32_t count;
            } outputs;

            vx_IrOpType op_type;

            struct {
                CompOperand * items;
                uint32_t count;
            } operands;
        } specific;
    };
} CompOperation;

typedef struct {
    uint32_t   placeholders_count;
    germanstr* placeholders;

    CompOperation * items;
    uint32_t        count;
} CompPattern;

/** if return 0, not enough space */
size_t Pattern_serialize(CompPattern pat, void* dest, size_t destzu);

uint32_t Pattern_placeholderId(CompPattern pat, const char * name);

// TODO: NAMEDD ARGS

/**
 * `...` means zero or more instructions
 *
 * \code
 * S = sub(B, C)
 * ...
 * A = add(S, C)
 * \endcode 
 *
 * \code 
 * B = load(A)
 * ...
 * B = add(B, 1)
 * ...
 * store(A, B)
 * \endcode
 */
CompPattern Pattern_compile(const char * source);
void Pattern_free(CompPattern);

/** is a range because of any matches; can be NULL (only for any matches) */
typedef struct {
    vx_IrOp* first;
    vx_IrOp* last;
} PatternInstMatch;

typedef struct {
    bool found;
    vx_IrVar* matched_placeholders;
    PatternInstMatch* matched_instrs;
    vx_IrOp* last;
} OptPatternMatch;

OptPatternMatch Pattern_matchNext(vx_IrOp* first, CompPattern pattern);
void OptPatternMatch_free(OptPatternMatch);
