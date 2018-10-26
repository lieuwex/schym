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
	if (fn->isBuiltin) {
		return fn->fn(scope, name, nargs, args);
	}

	size_t fn_nargs = fn->args.len;

	EXPECT(==, fn_nargs);

	for (size_t i = 0; i < fn_nargs; i++) {
		assert(i < nargs);
		RunResult rr = run(scope, args[i]);
		if (rr.err != NULL) {
			return rr;
		}
		varmap_setItem(scope->variables, fn->args.nodes[i]->var.name, node_copy(rr.node));
	}

	RunResult rr = run(scope, fn->body);
	if (rr.err != NULL) {
		return rr;
	}

	rr.node = node_copy(rr.node); // REVIEW
	return rr;
}

Node *getVar(const Scope *scope, const char *name) {
	while (scope != NULL) {
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
			return rr_errf("cannot call nil value");
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
			rr.node->function.isBuiltin ?
				scope :
				rr.node->function.scope,
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
