#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "./ast_manip.h"

#define malloc(n, type) (type*)malloc(n * sizeof(type))
#define realloc(ptr, count, type) (type*)realloc(ptr, count * sizeof(type))

Node *createNode(ASTtype type, bool quoted) {
	Node *res = malloc(1, Node);

	if (quoted) {
		res->type = AST_QUOTED;
		res->quoted.node = createNode(type, false);
		return res;
	}

	res->type = type;

	if (type == AST_EXPR) {
		res->expr.len = 0;
		res->expr.nodes = malloc(0, Node*);
	}

	return res;
}

bool expr_append(Node *list, const Node *node) {
	if (list->type == AST_QUOTED) {
		list = list->quoted.node;
	}
	assert(list->type == AST_EXPR);

	list->expr.len++;
	list->expr.nodes = realloc(list->expr.nodes, list->expr.len, Node*);

	list->expr.nodes[list->expr.len-1] = node_copy(node);
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

Node *array_to_list(const Node **nodes, size_t len) {
	Node *res = createNode(AST_EXPR, true);
	for (size_t i = 0; i < len; i++) {
		expr_append(res, nodes[i]);
	}
	return res;
}
