#include <stdlib.h>
#include "../src/nice_ast.h"

int main(void) {
	Node *node = createNode(AST_EXPR, true);

	for (size_t i = 0; i < 5; i++) {
		Node *toAdd = createNode(AST_NUM, false);
		toAdd->num.val = i+1;
		expr_append(node, toAdd);
	}

	printf("%s\n", stringify(node, 0));
	node_free(node);
}
