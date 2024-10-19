#include "parser.h"

int main()
{
    CompPattern p = Pattern_compile(
            "r = mul(a=a, b=b)"
       "\n" "o = add(a=r, b=c)"
            );

    Pattern_free(p);
}
