#include <stdbool.h>
#include <stdint.h>

#include "../ir/ir.h"
#include "../allib/germanstr/germanstr.h"


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
    bool is_any;

    union {
        struct {

        } any;

        struct {
            struct {
                size_t * items;
                size_t count;
            } outputs;

            vx_IrOpType op_type;

            struct {
                CompOperand * items;
                size_t count;
            } operands;
        } specific;
    };
} CompOperation;

typedef struct {
    size_t     placeholders_count;
    germanstr* placeholders;

    CompOperation * items;
    size_t count;
} CompPattern;

size_t Pattern_placeholderId(CompPattern pat, const char * name);

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
