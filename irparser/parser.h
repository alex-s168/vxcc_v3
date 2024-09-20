#include <stdbool.h>
#include <stdint.h>

#include "../ir/ir.h"


typedef enum {
    OPERAND_TYPE_PLACEHOLDER,
    OPERAND_TYPE_IMM_INT,
    OPERAND_TYPE_IMM_FLT,
} OperandType;

typedef struct {
    OperandType type;
    union {
        size_t placeholder;
        long long int imm_int;
        double imm_flt;
    } v;
} CompOperand;

typedef struct {
    struct {
        size_t * items;
        size_t count;
    } outputs;

    vx_IrOpType op_type;

    struct {
        CompOperand * items;
        size_t count;
    } operands;
} CompOperation;

typedef struct {
    size_t num_placeholders;

    CompOperation * items;
    size_t count;
} CompPattern;

/**
 * \code
 * S = sub(B, C)
 * A = add(S, C)
 * \endcode 
 *
 * \code 
 * B = load(A)
 * B = add(B, 1)
 * store(A, B)
 * \endcode
 */
CompPattern Pattern_compile(const char * source);
void Pattern_free(CompPattern);
