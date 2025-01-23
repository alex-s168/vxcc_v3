#include <stdio.h>
#include "sexpr.h"

int main()
{
	struct SNode* nd = snode_parse(stdin);
	if (!nd) return 1;
	snode_print(nd, stdout);
	snode_free(nd);
}
