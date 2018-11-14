#include <string.h>
#include "../../ast.h"
#include "../../util.h"
#include "../interpreter.h"
#include "lists.h"

static Node *bool_node(bool val) {
	Node *node = malloc(1, sizeof(Node));
	node->type = AST_NUM;
	node->num.val = val;
	return node;
}

static Node *makeList(size_t size) {
	Node *expr = malloc(1, sizeof(Node));
	expr->type = AST_EXPR;

	expr->expr.len = size;
	expr->expr.nodes = calloc(size, sizeof(Node*));

	Node *res = malloc(1, sizeof(Node));
	res->type = AST_QUOTED;
	res->quoted.node = expr;

	return res;
}

RunResult builtin_list(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	Node *res = makeList(nargs);

	for (size_t i = 0; i < nargs; i++) {
		const Node *node = args[i];
		RunResult rr = run(scope, node);

		if (rr.err != NULL) {
			return rr;
		}

		res->quoted.node->expr.nodes[i] = rr.node;
	}

	return rr_node(res);
}

RunResult builtin_car(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(==, 1);
	RunResult rr = run(scope, args[0]);
	if (rr.err != NULL) {
		return rr;
	}

	// REVIEW
	Node *node = rr.node->quoted.node->expr.nodes[0];
	return rr_node(node);
}

RunResult builtin_cdr(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;
	EXPECT(==, 1);
	RunResult rr = run(scope, args[0]);
	if (rr.err != NULL) {
		return rr;
	}

	// TODO: typecheck
	// REVIEW
	rr.node->quoted.node->expr.len--;
	rr.node->quoted.node->expr.nodes = rr.node->quoted.node->expr.nodes+1;
	return rr_node(rr.node);
}

// TODO: handle strings and then builtin concat into prelude
RunResult builtin_append(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;
	EXPECT(==, 2);

	RunResult list = run(scope, args[0]);
	if (list.err != NULL) {
		return list;
	}

	RunResult val = run(scope, args[1]);
	if (val.err != NULL) {
		return val;
	}

	if (list.node->type != AST_QUOTED) {
		return rr_errf("expected list");
	}

	list.node->quoted.node->expr.len++;
	list.node->quoted.node->expr.nodes = realloc(list.node->quoted.node->expr.nodes, list.node->quoted.node->expr.len, sizeof(Node*));
	list.node->quoted.node->expr.nodes[list.node->quoted.node->expr.len-1] = node_copy(val.node);

	return rr_null();
}

RunResult builtin_cons(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;
	EXPECT(==, 2);

	// TODO: do this how it's actually supposed to be.

	RunResult item = run(scope, args[0]);
	if (item.err != NULL) {
		return item;
	}

	RunResult list = run(scope, args[1]);
	if (list.err != NULL) {
		return list;
	}
	if (list.node->type != AST_QUOTED) {
		return rr_errf("expected list");
	}

	Node *listcpy = node_copy(list.node);
	Node *itemcpy = node_copy(item.node);

	listcpy->quoted.node->expr.len++;
	listcpy->quoted.node->expr.nodes = realloc(
		listcpy->quoted.node->expr.nodes,
		listcpy->quoted.node->expr.len,
		sizeof(Node*)
	);
	memmove(listcpy->quoted.node->expr.nodes + 1, listcpy->quoted.node->expr.nodes, listcpy->quoted.node->expr.len-1);
	listcpy->quoted.node->expr.nodes[0] = itemcpy;
}

RunResult builtin_null(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;
	EXPECT(==, 1);

	RunResult list = run(scope, args[1]);
	if (list.err != NULL) {
		return list;
	}
	if (list.node->type != AST_QUOTED) {
		return rr_node(bool_node(false));
	}

	const bool val = list.node->quoted.node->expr.len == 0;
	return rr_node(bool_node(val));
}

void init_builtins_lists(BuiltinList *ls) {
	addBuiltin(ls, "list", builtin_list);
	addBuiltin(ls, "car", builtin_car);
	addBuiltin(ls, "cdr", builtin_cdr);
	addBuiltin(ls, "append", builtin_append);
	addBuiltin(ls, "cons", builtin_cons);
	addBuiltin(ls, "null?", builtin_null);
}
