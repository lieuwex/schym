#include <strings.h>
#include <assert.h>
#include "../../ast.h"
#include "../../util.h"
#include "../interpreter.h"
#include "../../stringify.h"
#include "stdio.h"

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

static bool isQuoted(const Node *node, const char *str) {
	return (
		node->type == AST_QUOTED &&
		node->quoted.node->type == AST_VAR &&
		streq(node->quoted.node->var.name, str)
	);
}

RunResult builtin_print(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	RunResult res;
	char **strings = malloc(nargs, sizeof(char*));
	char *err = nodesToStrings(scope, nargs, args, strings);

	// TODO: raw mode should do more
	bool rawMode = nargs > 0 && isQuoted(args[0], "raw");

	if (err != NULL) {
		res = rr_errf(err);
	} else {
		size_t start = rawMode ? 1 : 0;
		for (size_t i = start; i < nargs; i++) {
			if (i != start) {
				putchar(' ');
			}

			printf("%s", strings[i]);
			free(strings[i]);
		}

		if (!rawMode) {
			putchar('\n');
		}
		res = rr_null();
	}

	free(strings);
	return res;
}

RunResult builtin_input(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)scope;
	(void)name;
	(void)args;

	EXPECT(==, 0);

	char *line = NULL;
	size_t len = 0;
	getline(&line, &len, stdin);

	Node *res = malloc(1, sizeof(Node));
	res->type = AST_STR;
	res->str.str = line;
	res->str.size = len;
	return rr_node(res);
}

RunResult builtin_load(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;
	EXPECT(==, 1);

	RunResult res;
	char **files = malloc(nargs, sizeof(char*));
	nodesToStrings(scope, nargs, args, files);

	char *input;
	for (int i = 0; i < 2; i++) {
		const char *prefix;
		switch (i) {
		case 0: // prelude
			prefix = "src/";
			break;
		case 1:
			// TODO: current dir actually
			prefix = "src/examples";
			break;
		}

		char *path;
		asprintf(&path, "%s%s.schym", prefix, files[0]); // TODO: smarter with .schym
		input = readfile(path);
		free(path);
		if (input != NULL) {
			break;
		}
	}

	if (input == NULL) {
		res = rr_errf("error while reading file");
	} else {
		res = runProgram(input, scope, true);
	}

	free(files);
	return res;
}

void init_builtins_stdio(BuiltinList *ls) {
	addBuiltin(ls, "print", builtin_print);
	addBuiltin(ls, "input", builtin_input);
	addBuiltin(ls, "load", builtin_load);
}
