#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "interpreter.h"
#include "internal.h"
#include "../stringify.h"
#include "../util.h"
#include "./builtins.h"

// TODO: some way to handle builtins

#define DEBUG 0

RunResult rr_null(void) {
	RunResult res = {
		.node = NULL,
		.err = NULL,
	};
	return res;
}

RunResult rr_errf(const char *format, ...) {
	// TODO: would be nice to know the location in the file where the error
	// occured.

	char *buf;

	va_list ap;
	va_start(ap, format);
	vasprintf(&buf, format, ap);
	va_end(ap);

	assert(buf);

	RunResult res = {
		.node = NULL,
		.err = buf,
	};
	return res;
}

RunResult rr_errfloc(Location loc, const char *format, ...) {
	// TODO: would be nice to know the location in the file where the error
	// occured.

	va_list ap;
	va_start(ap, format);
	RunResult res = rr_errf(format, ap);
	va_end(ap);

	char *cpy = astrcpy(res.err);
	free(res.err);
	asprintf(&res.err, "at line %d col %d: %s", loc.line, loc.col, cpy);
	free(cpy);

	return res;
}

RunResult rr_node(Node *node) {
	RunResult res = {
		.node = node,
		.err = NULL,
	};
	return res;
}

double getNumVal(Scope *scope, const Node *node) {
	switch (node->type) {
	case AST_NUM:
		return node->num.val;

	case AST_VAR:
	case AST_EXPR: {
		RunResult rr = run(scope, node);
		assert(rr.err == NULL);
		double res = getNumVal(scope, rr.node);
		if (rr.node != NULL) {
			node_free(rr.node);
		}
		return res;
	}

	case AST_STR:
		return (long)node->str.str;

	case AST_QUOTED:
		if (node->quoted.node->type == AST_VAR) {
			return (long)node->quoted.node->var.name;
		}
		assert(false);
		return 0;

	default:
		return 0;
	}
}

static RunResult runFunction(Scope *scope, const Function *fn, const char *name, const Node **args, size_t nargs) {
#if DEBUG
	printf("------- calling %s (scope=%p)\n", name, scope);
	varmap_print(scope->variables);
	puts("---------");
	puts("");
#endif

	if (fn->isBuiltin) {
		return fn->fn(scope, name, nargs, args);
	}

	size_t fn_nargs = fn->args.len;
	EXPECT(==, fn_nargs);

	Scope *newScope = scope_make(scope, false);
	RunResult res;

	for (size_t i = 0; i < fn_nargs; i++) {
		RunResult rr = run(scope, args[i]);
		if (rr.err != NULL) {
			res = rr;
			goto done;
		}

#if DEBUG
		printf("(scope %p) varmap_setItem(%s, %s, %s);\n", newScope, "newScope->variables", fn->args.nodes[i]->var.name, "rr.node");
#endif
		varmap_setItem(newScope->variables, fn->args.nodes[i]->var.name, rr.node);
	}

	res = run(newScope, fn->body);
	if (res.err != NULL) {
		goto done;
	}

	res.node = node_copy(res.node); // REVIEW

done:
	scope_free(newScope);
	return res;
}

Node *getVar(const Scope *scope, const char *name) {
#if DEBUG
	int x = 0;
#endif
	while (scope != NULL) {
#if DEBUG
		printf("getting %s (try %d) (scope %p)\n", name, ++x, scope);
#endif
		Node *res = varmap_getItem(scope->variables, name);
		if (res != NULL) {
			return res;
		}

		scope = scope->parent;
	}

	return NULL;
}

RunResult run(Scope *scope, const Node *node) {
	assert(scope);
	assert(node);

	switch (node->type) {
	case AST_QUOTED:
	case AST_STR:
	case AST_NUM: {
		Node *copy = node_copy(node);
		return rr_node(copy);
	}

	case AST_EXPR: {
		if (node->expr.len == 0) {
			return rr_errf("Non-quoted expression can't be empty");
		}

		RunResult rr = run(scope, node->expr.nodes[0]);
		if (rr.err != NULL) {
			return rr;
		}

		if (rr.node == NULL) {
			return rr_errf("cannot call nil value '%s'", stringify(node->expr.nodes[0], 0));
		} else if (rr.node->type != AST_FUN) {
			return rr_errf(
				"Cannot call non-function (type %s)",
				typetostr(node->expr.nodes[0])
			);
		}

		// REVIEW
		assert(
			!rr.node->function.isBuiltin ||
			node->expr.nodes[0]->type == AST_VAR
		);

		const Node **args = (const Node**)node->expr.nodes + 1;

		RunResult res = runFunction(
			scope,
			//rr.node->function.isBuiltin ?
			//	scope :
			//	rr.node->function.scope,
			&rr.node->function,
			node->expr.nodes[0]->var.name,
			args,
			node->expr.len - 1
		);
		return res;
	}

	case AST_VAR: {
		const Node *val = getVar(scope, node->var.name);
		if (val != NULL) {
			return rr_node(node_copy(val));
		}

		Scope *rootScope = scope_get_root(scope);
		Builtin *builtin = getBuiltin(rootScope->builtins, node->var.name);
		if (builtin != NULL) {
			// REVIEW
			Node *node = malloc(1, sizeof(Node));
			node->type = AST_FUN;
			node->function = builtin->function;
			return rr_node(node);
		}

		return rr_null();
	}

	case AST_COMMENT:
		return rr_null();

	default:
		assert(false);
	}
}

RunResult in_run(Scope *scope, const InternedNode node) {
	return run(scope, node.node);
}

RunResult runProgram(char *input, Scope *scope, bool doIntern) {
	RunResult res = rr_null();

	ProgramParseResult program = parseprogram(input);
	free(input);

	if (program.err) {
		char *err;
		asprintf(
			&err,
			"Program error: %s at line %d col %d\n",
			program.err,
			program.errloc.line,
			program.errloc.col
		);
		res.err = err;

		return res;
	}

	InternEnvironment *env = NULL;
	if (doIntern) {
		env = ie_make();
	}

	for (size_t i = 0; i < program.len; i++) {
		if (program.nodes[i]->type != AST_EXPR) {
			continue;
		}

		InternedNode interned;
		if (doIntern) {
			interned = intern(program.nodes[i], env);
		} else {
			interned = skipIntern(program.nodes[i]);
		}

		res = in_run(scope, interned);
		node_free(interned.node);

		if (res.err != NULL) {
			char *err;
			asprintf(&err, "Error while executing code: %s\n", res.err);
			res.err = err;

			return res;
		}
	}

	ie_free(env, false);
	return res;
}
