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

InterEnv* in_make(void) {
	InterEnv *in = malloc(1,sizeof(InterEnv));
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

static RunResult funcCall(InterEnv *env, const char *name, const Node **args, size_t nargs) {
	const Builtin *builtin = getBuiltin(name);
	if (builtin != NULL) {
		return builtin->fn(env, name, nargs, args);
	}

	// TODO: check variables

	return rr_errf("no function '%s' found", name);
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
		} else if (node->expr.nodes[0]->type != AST_VAR) {
			return rr_errf(
				"Cannot call non-variable (type %s)",
				typetostr(node->expr.nodes[0])
			);
		}

		const Node **args = (const Node**)node->expr.nodes + 1;
		return funcCall(
			env,
			node->expr.nodes[0]->var.name,
			args,
			node->expr.len - 1
		);
	}

	case AST_VAR: {
		const Node *val = varmap_getItem(env->variables, node->var.name);
		if (val == NULL) {
			return rr_null();
		} else {
			return rr_node(node_copy(val));
		}
	}

	case AST_COMMENT:
		return rr_null();

	default:
		assert(false);
	}
}

RunResult in_run(InterEnv* env, const InternedNode node) {
	return run(env, node.node);
}
