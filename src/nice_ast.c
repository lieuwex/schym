#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "./nice_ast.h"

#define malloc(n, sz) malloc(n * sz)

Node *createNode(ASTtype type, bool quoted) {
	Node *res = malloc(1, sizeof(Node));

	if (quoted) {
		res->type = AST_QUOTED;
		res->quoted.node = createNode(type, false);
		return res;
	}

	res->type = type;

	if (type == AST_EXPR) {
		res->expr.len = 0;
		res->expr.nodes = malloc(0, 0);
	}

	return res;
}

bool expr_append(Node *list, Node *node) {
	if (list->type == AST_QUOTED) {
		list = list->quoted.node;
	}
	assert(list->type == AST_EXPR);

	list->expr.len++;
	list->expr.nodes = realloc(list->expr.nodes, list->expr.len*sizeof(Node*));

	list->expr.nodes[list->expr.len-1] = node;
	return true; // REVIEW: how do we know the realloc failed?
}

bool expr_remove(Node *node, size_t at) {
	if (node->type == AST_QUOTED) {
		node = node->quoted.node;
	}
	assert(node->type == AST_EXPR);

	if (at >= node->expr.len) {
		return false;
	}

	node_free(node->expr.nodes[at]);
	memmove(
		node->expr.nodes + at,
		node->expr.nodes + at + 1,
		(node->expr.len-at-1) * sizeof(Node*)
	);
	node->expr.len--;

	return true;
}
