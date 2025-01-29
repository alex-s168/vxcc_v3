#include "../../ir/ir.h"

#define ELF_RELOC(n,v) static uint8_t n = v;

ELF_RELOC(R_RISCV_NONE,               0)
ELF_RELOC(R_RISCV_32,                 1)
ELF_RELOC(R_RISCV_64,                 2)
ELF_RELOC(R_RISCV_RELATIVE,           3)
ELF_RELOC(R_RISCV_COPY,               4)
ELF_RELOC(R_RISCV_JUMP_SLOT,          5)
ELF_RELOC(R_RISCV_TLS_DTPMOD32,       6)
ELF_RELOC(R_RISCV_TLS_DTPMOD64,       7)
ELF_RELOC(R_RISCV_TLS_DTPREL32,       8)
ELF_RELOC(R_RISCV_TLS_DTPREL64,       9)
ELF_RELOC(R_RISCV_TLS_TPREL32,       10)
ELF_RELOC(R_RISCV_TLS_TPREL64,       11)
ELF_RELOC(R_RISCV_TLSDESC,           12)
ELF_RELOC(R_RISCV_BRANCH,            16)
ELF_RELOC(R_RISCV_JAL,               17)
ELF_RELOC(R_RISCV_CALL,              18)
ELF_RELOC(R_RISCV_CALL_PLT,          19)
ELF_RELOC(R_RISCV_GOT_HI20,          20)
ELF_RELOC(R_RISCV_TLS_GOT_HI20,      21)
ELF_RELOC(R_RISCV_TLS_GD_HI20,       22)
ELF_RELOC(R_RISCV_PCREL_HI20,        23)
ELF_RELOC(R_RISCV_PCREL_LO12_I,      24)
ELF_RELOC(R_RISCV_PCREL_LO12_S,      25)
ELF_RELOC(R_RISCV_HI20,              26)
ELF_RELOC(R_RISCV_LO12_I,            27)
ELF_RELOC(R_RISCV_LO12_S,            28)
ELF_RELOC(R_RISCV_TPREL_HI20,        29)
ELF_RELOC(R_RISCV_TPREL_LO12_I,      30)
ELF_RELOC(R_RISCV_TPREL_LO12_S,      31)
ELF_RELOC(R_RISCV_TPREL_ADD,         32)
ELF_RELOC(R_RISCV_ADD8,              33)
ELF_RELOC(R_RISCV_ADD16,             34)
ELF_RELOC(R_RISCV_ADD32,             35)
ELF_RELOC(R_RISCV_ADD64,             36)
ELF_RELOC(R_RISCV_SUB8,              37)
ELF_RELOC(R_RISCV_SUB16,             38)
ELF_RELOC(R_RISCV_SUB32,             39)
ELF_RELOC(R_RISCV_SUB64,             40)
ELF_RELOC(R_RISCV_GOT32_PCREL,       41)
ELF_RELOC(R_RISCV_ALIGN,             43)
ELF_RELOC(R_RISCV_RVC_BRANCH,        44)
ELF_RELOC(R_RISCV_RVC_JUMP,          45)
ELF_RELOC(R_RISCV_RELAX,             51)
ELF_RELOC(R_RISCV_SUB6,              52)
ELF_RELOC(R_RISCV_SET6,              53)
ELF_RELOC(R_RISCV_SET8,              54)
ELF_RELOC(R_RISCV_SET16,             55)
ELF_RELOC(R_RISCV_SET32,             56)
ELF_RELOC(R_RISCV_32_PCREL,          57)
ELF_RELOC(R_RISCV_IRELATIVE,         58)
ELF_RELOC(R_RISCV_PLT32,             59)
ELF_RELOC(R_RISCV_SET_ULEB128,       60)
ELF_RELOC(R_RISCV_SUB_ULEB128,       61)
ELF_RELOC(R_RISCV_TLSDESC_HI20,      62)
ELF_RELOC(R_RISCV_TLSDESC_LOAD_LO12, 63)
ELF_RELOC(R_RISCV_TLSDESC_ADD_LO12,  64)
ELF_RELOC(R_RISCV_TLSDESC_CALL,      65)
ELF_RELOC(R_RISCV_VENDOR,           191)
ELF_RELOC(R_RISCV_CUSTOM192,        192)
ELF_RELOC(R_RISCV_CUSTOM193,        193)
ELF_RELOC(R_RISCV_CUSTOM194,        194)
ELF_RELOC(R_RISCV_CUSTOM195,        195)
ELF_RELOC(R_RISCV_CUSTOM196,        196)
ELF_RELOC(R_RISCV_CUSTOM197,        197)
ELF_RELOC(R_RISCV_CUSTOM198,        198)
ELF_RELOC(R_RISCV_CUSTOM199,        199)
ELF_RELOC(R_RISCV_CUSTOM200,        200)
ELF_RELOC(R_RISCV_CUSTOM201,        201)
ELF_RELOC(R_RISCV_CUSTOM202,        202)
ELF_RELOC(R_RISCV_CUSTOM203,        203)
ELF_RELOC(R_RISCV_CUSTOM204,        204)
ELF_RELOC(R_RISCV_CUSTOM205,        205)
ELF_RELOC(R_RISCV_CUSTOM206,        206)
ELF_RELOC(R_RISCV_CUSTOM207,        207)
ELF_RELOC(R_RISCV_CUSTOM208,        208)
ELF_RELOC(R_RISCV_CUSTOM209,        209)
ELF_RELOC(R_RISCV_CUSTOM210,        210)
ELF_RELOC(R_RISCV_CUSTOM211,        211)
ELF_RELOC(R_RISCV_CUSTOM212,        212)
ELF_RELOC(R_RISCV_CUSTOM213,        213)
ELF_RELOC(R_RISCV_CUSTOM214,        214)
ELF_RELOC(R_RISCV_CUSTOM215,        215)
ELF_RELOC(R_RISCV_CUSTOM216,        216)
ELF_RELOC(R_RISCV_CUSTOM217,        217)
ELF_RELOC(R_RISCV_CUSTOM218,        218)
ELF_RELOC(R_RISCV_CUSTOM219,        219)
ELF_RELOC(R_RISCV_CUSTOM220,        220)
ELF_RELOC(R_RISCV_CUSTOM221,        221)
ELF_RELOC(R_RISCV_CUSTOM222,        222)
ELF_RELOC(R_RISCV_CUSTOM223,        223)
ELF_RELOC(R_RISCV_CUSTOM224,        224)
ELF_RELOC(R_RISCV_CUSTOM225,        225)
ELF_RELOC(R_RISCV_CUSTOM226,        226)
ELF_RELOC(R_RISCV_CUSTOM227,        227)
ELF_RELOC(R_RISCV_CUSTOM228,        228)
ELF_RELOC(R_RISCV_CUSTOM229,        229)
ELF_RELOC(R_RISCV_CUSTOM230,        230)
ELF_RELOC(R_RISCV_CUSTOM231,        231)
ELF_RELOC(R_RISCV_CUSTOM232,        232)
ELF_RELOC(R_RISCV_CUSTOM233,        233)
ELF_RELOC(R_RISCV_CUSTOM234,        234)
ELF_RELOC(R_RISCV_CUSTOM235,        235)
ELF_RELOC(R_RISCV_CUSTOM236,        236)
ELF_RELOC(R_RISCV_CUSTOM237,        237)
ELF_RELOC(R_RISCV_CUSTOM238,        238)
ELF_RELOC(R_RISCV_CUSTOM239,        239)
ELF_RELOC(R_RISCV_CUSTOM240,        240)
ELF_RELOC(R_RISCV_CUSTOM241,        241)
ELF_RELOC(R_RISCV_CUSTOM242,        242)
ELF_RELOC(R_RISCV_CUSTOM243,        243)
ELF_RELOC(R_RISCV_CUSTOM244,        244)
ELF_RELOC(R_RISCV_CUSTOM245,        245)
ELF_RELOC(R_RISCV_CUSTOM246,        246)
ELF_RELOC(R_RISCV_CUSTOM247,        247)
ELF_RELOC(R_RISCV_CUSTOM248,        248)
ELF_RELOC(R_RISCV_CUSTOM249,        249)
ELF_RELOC(R_RISCV_CUSTOM250,        250)
ELF_RELOC(R_RISCV_CUSTOM251,        251)
ELF_RELOC(R_RISCV_CUSTOM252,        252)
ELF_RELOC(R_RISCV_CUSTOM253,        253)
ELF_RELOC(R_RISCV_CUSTOM254,        254)
ELF_RELOC(R_RISCV_CUSTOM255,        255)
