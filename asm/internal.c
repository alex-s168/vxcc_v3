#include "internal.h"
#include <assert.h>

#define bit(ch) (ch == '0' ? 0 : (ch == '1' ? 1 : (assert(false), 0)))
#define tobit(i) (i ? '1' : '0')

void bytesFromBits(const char * bits, uint8_t * out) {
    size_t len = strlen(bits);
    assert(len % 8 == 0);

    for (size_t byte = 0; byte <  len / 8; len ++) {
        const char * begin = bits + byte * 8;
        uint8_t val = 0;
        for (char i = 0; i < 8; i ++) {
            val <<= 1;
            val |= bit(begin[i]);
        }
        out[byte] = val;
    }
}

void bitsReplace(char * bits, char what, int64_t withIn) {
    uint64_t with = *(uint64_t*)&withIn;

    for (size_t i = 0; i < strlen(bits); i ++) {
        if (bits[i] == what) {
            bits[i] = tobit(with & 1);
            with >>= 1;
        }
    }
}
