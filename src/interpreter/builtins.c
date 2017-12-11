#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <strings.h>
#include "../ast.h"
#include "internal.h"
#include "../stringify.h"
#include "../util.h"
#include "./builtins.h"

#define EXPECT_TYPE(narg, typ) do { \
	if (args[(narg)]->type != (typ)) { \
		return rr_errf("expected argument %d to be of type %s", narg, #typ); \
	} \
} while(0)

static char *nodesToStrings(InterEnv *env, size_t nargs, const Node **args, char **output) {
	for (size_t i = 0; i < nargs; i++) {
		const Node *node = args[i];
		assert(node);

		RunResult rr = run(env, node);
		if (rr.err != NULL) {
			return rr.err;
		}

		output[i] = toString(rr.node);
	}

	return NULL;
}

static InterEnv *createScope(InterEnv *parent) {
	InterEnv *res = in_make();
	res->parent = parent;
	return res;
}

RunResult builtin_print(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	(void)name;
	EXPECT(>=, 1);

	RunResult res;
	char **strings = malloc(nargs, sizeof(char*));
	char *err = nodesToStrings(env, nargs, args, strings);

	if (err != NULL) {
		res = rr_errf(err);
	} else {
		for (size_t i = 0; i < nargs; i++) {
			if (i != 0) {
				putchar(' ');
			}

			printf("%s", strings[i]);
			free(strings[i]);
		}

		putchar('\n'); // REVIEW: do we want to do this?
		res = rr_null();
	}

	free(strings);
	return res;
}

RunResult builtin_arith(InterEnv *env, const char *name, size_t nargs, const Node **args) {
#define CHECKNUM(node) do { \
	if (node == NULL || node->type != AST_NUM) { \
		return rr_errf("all arguments should be a number"); \
	} \
} while(0);

	EXPECT(>=, 2);

	Node *res = malloc(1, sizeof(Node));
	res->type = AST_NUM;

	RunResult rr = run(env, args[0]);
	CHECKNUM(rr.node);
	res->num.val = rr.node->num.val;

	for (size_t i = 1; i < nargs; i++) {
		const RunResult rr = run(env, args[i]);
		CHECKNUM(rr.node);
		const double n = rr.node->num.val;

		switch (name[0]) {
		case '+':
			res->num.val += n;
			break;
		case '-':
			res->num.val -= n;
			break;
		case '/':
			res->num.val /= n;
			break;
		case '*':
			res->num.val *= n;
			break;
		case '^':
			res->num.val = pow(res->num.val, n);
			break;
		}
	}

	return rr_node(res);

#undef CHECKNUM
}

RunResult builtin_comp(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	EXPECT(==, 2);

	Node *res = malloc(1, sizeof(Node));
	res->type = AST_NUM;

	double n1 = getNumVal(env, args[0]);
	double n2 = getNumVal(env, args[1]);

	if (streq(name, "==")) {
		res->num.val = n1 == n2;
	} else if (streq(name, "!=")) {
		res->num.val = n1 != n2;
	} else if (streq(name, "<")) {
		res->num.val = n1 < n2;
	} else if (streq(name, ">")) {
		res->num.val = n1 > n2;
	} else if (streq(name, "<=")) {
		res->num.val = n1 <= n2;
	} else if (streq(name, ">=")) {
		res->num.val = n1 >= n2;
	}

	return rr_node(res);
}

RunResult builtin_do(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(>=, 1);

	RunResult rr;
	for (size_t i = 0; i < nargs; i++) {
		rr = run(env, args[i]);
		if (rr.err != NULL) {
			return rr;
		}
	}

	return rr;
}

RunResult builtin_if(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(>=, 2);
	EXPECT(<=, 3);

	RunResult cond_rr = run(env, args[0]);
	if (cond_rr.err != NULL) {
		return cond_rr;
	} else if (cond_rr.node->type != AST_NUM) {
		return rr_errf("expected cond to have type AST_NUM");
	}

	if (cond_rr.node->num.val) {
		return run(env, args[1]);
	} else if (nargs == 3) {
		return run(env, args[2]);
	}

	return rr_null();
}

Node *makeVar(const char *name) {
	Node *res = malloc(1, sizeof(Node));
	res->type = AST_VAR;
	res->var.name = astrcpy(name);
	return res;
}

RunResult builtin_fun(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	(void)env;
	(void)name;

	EXPECT(>=, 2);
	EXPECT_TYPE(0, AST_EXPR);

	Node *node = malloc(1, sizeof(Node));
	node->type = AST_FUN;
	node->function.isBuiltin = false;
	node->function.env = createScope(env);

	size_t fn_nargs = args[0]->expr.len;
	node->function.args.len = fn_nargs;
	node->function.args.nodes = malloc(fn_nargs, sizeof(Node*));
	for (size_t i = 0; i < fn_nargs; i++) {
		assert(args[0]->expr.nodes[i]->type == AST_VAR); // TODO
		node->function.args.nodes[i] = node_copy(args[0]->expr.nodes[i]);
	}

	Node *body = malloc(1, sizeof(Node));
	body->type = AST_EXPR;
	body->expr.len = nargs; // we have +1 for the 'do'
	body->expr.nodes = malloc(nargs, sizeof(Node*));
	body->expr.nodes[0] = makeVar("do");
	for (size_t i = 1; i < nargs; i++) {
		body->expr.nodes[i] = node_copy(args[i]);
	}
	node->function.body = body;

	return rr_node(node);
}

RunResult builtin_set(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(>=, 2);
	EXPECT_TYPE(0, AST_VAR);

	RunResult rr;
	if (nargs == 2) { // value
		rr = run(env, args[1]);
	} else { // lambda
		rr = builtin_fun(env, "fun", nargs-1, args+1);
	}

	if (rr.err != NULL) {
		return rr;
	}

	while (env->parent != NULL) {
		const Node *node = varmap_getItem(env->variables, name);
		if (node != NULL) {
			break;
		}
		env = env->parent;
	}

	if (rr.node == NULL) {
		varmap_removeItem(env->variables, args[0]->var.name);
	} else {
		varmap_setItem(env->variables, args[0]->var.name, rr.node);
	}

	return rr_null();
}

RunResult builtin_let(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(>=, 3);

	InterEnv *scope = createScope(env);
	builtin_set(scope, "set", 2, args);

	RunResult rr;
	for (size_t i = 2; i < nargs; i++) {
		rr = run(scope, args[i]);
		if (rr.err != NULL) {
			return rr;
		}
	}

	in_destroy(scope);

	return rr;
}

RunResult builtin_streq(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(==, 2);

	Node *res = malloc(1, sizeof(Node));
	res->type = AST_NUM;

	RunResult rr1 = run(env, args[0]);
	RunResult rr2 = run(env, args[1]);

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

Node *mkQuotedExpr(size_t len) {
	Node *res = malloc(1, sizeof(Node));
	res->type = AST_QUOTED;

	res->quoted.node = malloc(1, sizeof(Node));
	res->quoted.node->type = AST_EXPR;

	res->quoted.node->expr.len = len;
	res->quoted.node->expr.nodes = malloc(len, sizeof(Node*));

	return res;
}

RunResult builtin_concat(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	(void)name;

	RunResult res;
	char **strings = malloc(nargs, sizeof(char*));
	char *err = nodesToStrings(env, nargs, args, strings);

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

RunResult builtin_times(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(>=, 3);
	EXPECT_TYPE(0, AST_VAR);
	EXPECT_TYPE(1, AST_EXPR);

	const Node *fromNode = args[1]->expr.nodes[0];
	assert(fromNode->num.val == (int)fromNode->num.val);
	const Node *toNode = args[1]->expr.nodes[1];
	assert(toNode->num.val == (int)toNode->num.val);

	RunResult rr;

	for (int i = fromNode->num.val; i < toNode->num.val; i++) {
		Node *node = malloc(1, sizeof(Node));
		node->type = AST_NUM;
		node->num.val = i;
		varmap_setItem(env->variables, args[0]->var.name, node);
		node_free(node);

		for (size_t j = 2; j < nargs; j++) {
			rr = run(env, args[j]);
			if (rr.err != NULL) {
				goto exit;
			}
		}
	}

exit:
	varmap_removeItem(env->variables, args[0]->var.name);
	return rr;
}

RunResult builtin_eval(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(==, 1);

	RunResult rr = run(env, args[0]);
	if (rr.err != NULL) {
		return rr;
	} else if (rr.node->type != AST_STR) {
		return rr_errf("expected expr to have type AST_STR");
	}

	const char *src = rr.node->str.str;

	ProgramParseResult program = parseprogram(src);
	if (program.err) {
		return rr_errf("Program error: %s at line %d col %d\n", program.err, program.errloc.line, program.errloc.col);
	} else if (program.len == 0) {
		return rr_errf("Program is empty\n");
	}

	RunResult res;
	for (size_t i = 0; i < program.len; i++) {
		res = in_run(env, skipIntern(program.nodes[i]));
		if (res.err != NULL) {
			break;
		}
	}
	return res;
}

RunResult builtin_assert(InterEnv *env, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(==, 1);

	RunResult rr = run(env, args[0]);
	if (rr.err != NULL) {
		return rr;
	} else if (rr.node->type != AST_NUM) {
		return rr_errf("expected cond to have type AST_NUM");
	}

	if (!rr.node->num.val) {
		char *str = stringify(args[0], 0);
		fprintf(stderr, "assertion failed: %s (result was %g)\n", str, rr.node->num.val);
		free(str);
		abort();
	}

	return rr_null();
}

static Builtin makeBuiltin(const char *name, RunResult (*fn)(InterEnv*, const char*, size_t, const Node**)) {
	Builtin res;

	res.name = name;
	res.enabled = true;

	res.function.isBuiltin = true;
	res.function.fn = fn;

	return res;
}

struct BuiltinList {
	size_t cap;
	size_t len;
	Builtin *items;
};
BuiltinList builtins;

void addBuiltin(const char *name, BuiltinFn fn) {
	builtins.len++;
	if (builtins.cap < builtins.len) {
		builtins.cap *= 2;
		builtins.items = realloc(builtins.items, builtins.cap, sizeof(Builtin));
	}
	builtins.items[builtins.len - 1] = makeBuiltin(name, fn);
}

static bool setup = false;

void setBuiltins(void) {
	builtins.len = 0;
	builtins.cap = 1;
	builtins.items = calloc(builtins.cap, sizeof(Builtin));

	addBuiltin("print", builtin_print);
	addBuiltin("+", builtin_arith);
	addBuiltin("-", builtin_arith);
	addBuiltin("/", builtin_arith);
	addBuiltin("*", builtin_arith);
	addBuiltin("^", builtin_arith);
	addBuiltin("==", builtin_comp);
	addBuiltin("!=", builtin_comp);
	addBuiltin("<", builtin_comp);
	addBuiltin(">", builtin_comp);
	addBuiltin("<=", builtin_comp);
	addBuiltin(">=", builtin_comp);
	addBuiltin("do", builtin_do);
	addBuiltin("if", builtin_if);
	addBuiltin("set", builtin_set);
	addBuiltin("let", builtin_let);
	addBuiltin("streq", builtin_streq);
	addBuiltin("fun", builtin_fun);
	addBuiltin("concat", builtin_concat);
	addBuiltin("times", builtin_times);
	addBuiltin("eval", builtin_eval);
	addBuiltin("assert", builtin_assert);
}

Builtin *getBuiltin(const char *name) {
	if (!setup) {
		setBuiltins(); // HACK
	}

	for (size_t i = 0; i < builtins.len; i++) {
		Builtin *builtin = builtins.items + i;
		if (builtin->enabled && streq(name, builtin->name)) {
			return builtin;
		}
	}

	return NULL;
}

void enableBuiltin(const char *name, bool enable) {
	getBuiltin(name)->enabled = enable;
}
