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

InterEnv *in_make(void) {
	InterEnv *in = malloc(1, sizeof(InterEnv));
	assert(in);
	in->variables = varmap_make();
	assert(in->variables);
	return in;
}

void in_destroy(InterEnv *is) {
	varmap_free(is->variables);
	free(is);
}

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

RunResult rr_node(Node *node) {
	RunResult res = {
		.node = node,
		.err = NULL,
	};
	return res;
}

double getNumVal(InterEnv *env, const Node *node) {
	switch (node->type) {
	case AST_NUM:
		return node->num.val;

	case AST_VAR:
	case AST_EXPR: {
		RunResult rr = run(env, node);
		assert(rr.err == NULL);
		double res = getNumVal(env, rr.node);
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

static RunResult runFunction(InterEnv *env, const Function *fn, const char *name, const Node **args, size_t nargs) {
	if (fn->isBuiltin) {
		return fn->fn(env, name, nargs, args);
	}

	size_t fn_nargs = fn->args.len;

	for (size_t i = 0; i < fn_nargs; i++) {
		assert(i < nargs);
		RunResult rr = run(env, args[i]);
		if (rr.err != NULL) {
			goto exit;
		}
		varmap_setItem(env->variables, fn->args.nodes[i]->var.name, node_copy(rr.node));
	}

	RunResult rr = run(env, fn->body);
	if (rr.err != NULL) {
		goto exit;
	}

exit:
	for (size_t i = 0; i < fn_nargs; i++) {
		varmap_removeItem(env->variables, fn->args.nodes[i]->var.name);
	}

	rr.node = node_copy(rr.node); // REVIEW
	return rr;
}

RunResult run(InterEnv *env, const Node *node) {
	assert(env);
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

		RunResult rr = run(env, node->expr.nodes[0]);
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
		return runFunction(
			env,
			&rr.node->function,
			node->var.name,
			args,
			node->expr.len - 1
		);
	}

	case AST_VAR: {
		const Node *val = varmap_getItem(env->variables, node->var.name);
		if (val != NULL) {
			return rr_node(node_copy(val));
		}

		Builtin *builtin = getBuiltin(node->var.name);
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

RunResult in_run(InterEnv *env, const InternedNode node) {
	return run(env, node.node);
}
