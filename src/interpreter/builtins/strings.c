#include <strings.h>
#include <assert.h>
#include "../../ast.h"
#include "../../util.h"
#include "../interpreter.h"
#include "../../stringify.h"
#include "strings.h"

static char *nodesToStrings(Scope *scope, size_t nargs, const Node **args, char **output) {
	for (size_t i = 0; i < nargs; i++) {
		const Node *node = args[i];
		assert(node);

		RunResult rr = run(scope, node);
		if (rr.err != NULL) {
			return rr.err;
		}

		output[i] = toString(rr.node);
	}

	return NULL;
}

RunResult builtin_streq(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(==, 2);

	Node *res = malloc(1, sizeof(Node));
	res->type = AST_NUM;

	// TODO: using `run` copies the string, so making the intern equality check
	// always ending up being `false`, should find something for that.
	RunResult rr1 = run(scope, args[0]);
	RunResult rr2 = run(scope, args[1]);

	if (
		(rr1.node->type != AST_STR) ||
		(rr2.node->type != AST_STR)
	) {
		return rr_errf("both arguments should be a string");
	}

	res->num.val = (
		rr1.node->str.str == rr2.node->str.str ||
		streq(rr1.node->str.str, rr2.node->str.str)
	);

	return rr_node(res);
}

RunResult builtin_concat(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	RunResult res;
	char **strings = malloc(nargs, sizeof(char*));
	char *err = nodesToStrings(scope, nargs, args, strings);

	if (err != NULL) {
		res = rr_errf(err);
	} else {
		char *buf = emptystr();

		for (size_t i = 0; i < nargs; i++) {
			strappend(&buf, strings[i]);
			free(strings[i]);
		}

		Node *node = malloc(1, sizeof(Node));
		node->type = AST_STR;
		node->str.size = strlen(buf);
		node->str.str = buf;

		res = rr_node(node);
	}

	free(strings);
	return res;
}

RunResult builtin_to_number(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(==, 1);
	RunResult rr = run(scope, args[0]);
	if (rr.err != NULL) {
		return rr;
	}

	switch (rr.node->type) {
	case AST_NUM:
		return rr_node(node_copy(rr.node));

	case AST_STR: {
		ParseResult pr = parse(rr.node->str.str);
		assert(pr.err == NULL && pr.node != NULL && pr.node->type == AST_NUM);
		return rr_node(pr.node);
	}

	default:
		return rr_errf("expected string or number, got %s", typetostr(rr.node));
	}
}

RunResult builtin_to_string(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)scope;
	(void)name;

	EXPECT(==, 1);
	RunResult rr = run(scope, args[0]);
	if (rr.err != NULL) {
		return rr;
	}

	char *str = toString(rr.node);
	node_free(rr.node);

	Node *node = malloc(1, sizeof(Node));
	node->type = AST_STR;
	node->str.str = str;
	node->str.size = strlen(str);
	return rr_node(node);
}

void init_builtins_strings(BuiltinList *ls) {
	addBuiltin(ls, "streq", builtin_streq);
	addBuiltin(ls, "concat", builtin_concat);
	addBuiltin(ls, "to-number", builtin_to_number);
	addBuiltin(ls, "to-string", builtin_to_string);
}
