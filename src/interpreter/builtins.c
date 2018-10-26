#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <strings.h>
#include "../ast.h"
#include "internal.h"
#include "../stringify.h"
#include "../util.h"
#include "./builtins.h"
#include "interpreter.h"

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

RunResult builtin_arith(Scope *scope, const char *name, size_t nargs, const Node **args) {
#define CHECKNUM(node) do { \
	if (node == NULL || node->type != AST_NUM) { \
		return rr_errf("all arguments should be a number"); \
	} \
} while(0);

	EXPECT(>=, 2);

	Node *res = malloc(1, sizeof(Node));
	res->type = AST_NUM;

	RunResult rr = run(scope, args[0]);
	CHECKNUM(rr.node);
	res->num.val = rr.node->num.val;

	for (size_t i = 1; i < nargs; i++) {
		const RunResult rr = run(scope, args[i]);
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

RunResult builtin_comp(Scope *scope, const char *name, size_t nargs, const Node **args) {
	EXPECT(==, 2);

	Node *res = malloc(1, sizeof(Node));
	res->type = AST_NUM;

	double n1 = getNumVal(scope, args[0]);
	double n2 = getNumVal(scope, args[1]);

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

RunResult builtin_do(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(>=, 1);

	RunResult rr;
	for (size_t i = 0; i < nargs; i++) {
		rr = run(scope, args[i]);
		if (rr.err != NULL) {
			return rr;
		}
	}

	return rr;
}

RunResult builtin_if(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(>=, 2);
	EXPECT(<=, 3);

	RunResult cond_rr = run(scope, args[0]);
	if (cond_rr.err != NULL) {
		return cond_rr;
	} else if (cond_rr.node->type != AST_NUM) {
		return rr_errf("expected cond to have type AST_NUM");
	}

	if (cond_rr.node->num.val) {
		return run(scope, args[1]);
	} else if (nargs == 3) {
		return run(scope, args[2]);
	}

	return rr_null();
}

Node *makeVar(const char *name) {
	Node *res = malloc(1, sizeof(Node));
	res->type = AST_VAR;
	res->var.name = astrcpy(name);
	return res;
}

RunResult builtin_fun(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)scope;
	(void)name;

	EXPECT(>=, 2);
	EXPECT_TYPE(0, AST_EXPR);

	Node *node = malloc(1, sizeof(Node));
	node->type = AST_FUN;
	node->function.isBuiltin = false;
	node->function.scope = scope_make(scope, true);

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

RunResult builtin_set(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(>=, 2);
	EXPECT_TYPE(0, AST_VAR);

	RunResult rr;
	if (nargs == 2) { // value
		rr = run(scope, args[1]);
	} else { // lambda
		rr = builtin_fun(scope, "fun", nargs-1, args+1);
	}

	if (rr.err != NULL) {
		return rr;
	}

	// REVIEW: what the fuck are we doing here lol
	while (scope->parent != NULL) {
		const Node *node = varmap_getItem(scope->variables, name);
		if (node != NULL) {
			break;
		}
		scope = scope->parent;
	}

	if (rr.node == NULL) {
		varmap_removeItem(scope->variables, args[0]->var.name);
	} else {
		varmap_setItem(scope->variables, args[0]->var.name, rr.node);
	}

	return rr_null();
}

RunResult builtin_let(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(>=, 2);

	Scope *newScope = scope_make(scope, true);

	EXPECT_TYPE(0, AST_EXPR);
	for (size_t i = 0; i < args[0]->expr.len; i++) {
		const Node *node = args[0]->expr.nodes[i];
		if (node->type != AST_EXPR) {
			RunResult rr = run(scope, node);
			if (rr.err != NULL) {
				return rr;
			}
			node = rr.node;
		}

		const Expression *pair = &node->expr;
		assert(pair->len == 2); // TODO

		RunResult rr = run(scope, pair->nodes[1]);
		if (rr.err != NULL) {
			return rr;
		}

		const char *name = pair->nodes[0]->var.name;
		const Node *val = rr.node;
		varmap_setItem(newScope->variables, name, val);
	}

	RunResult rr;
	for (size_t i = 1; i < nargs; i++) {
		rr = run(newScope, args[i]);
		if (rr.err != NULL) {
			return rr;
		}
	}

	// TODO: this should be freed, but it can be binded to a function.
	// I have now idea what a simple way to fix this should be.
	//scope_free(newScope);

	return rr;
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

Node *mkQuotedExpr(size_t len) {
	Node *res = malloc(1, sizeof(Node));
	res->type = AST_QUOTED;

	res->quoted.node = malloc(1, sizeof(Node));
	res->quoted.node->type = AST_EXPR;

	res->quoted.node->expr.len = len;
	res->quoted.node->expr.nodes = malloc(len, sizeof(Node*));

	return res;
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

RunResult builtin_times(Scope *scope, const char *name, size_t nargs, const Node **args) {
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
		varmap_setItem(scope->variables, args[0]->var.name, node);
		node_free(node);

		for (size_t j = 2; j < nargs; j++) {
			rr = run(scope, args[j]);
			if (rr.err != NULL) {
				goto exit;
			}
		}
	}

exit:
	varmap_removeItem(scope->variables, args[0]->var.name);
	return rr;
}

RunResult builtin_eval(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(==, 1);

	RunResult rr = run(scope, args[0]);
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
		res = in_run(scope, skipIntern(program.nodes[i]));
		if (res.err != NULL) {
			break;
		}
	}
	return res;
}

RunResult builtin_assert(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	EXPECT(==, 1);

	RunResult rr = run(scope, args[0]);
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

RunResult builtin_cond(Scope *scope, const char *name, size_t nargs, const Node **args) {
	(void)name;

	for (size_t i = 0; i < nargs; i++) {
		EXPECT_TYPE(i, AST_EXPR);

		const Expression *expr = &args[i]->expr;
		RunResult cond_rr = run(scope, expr->nodes[0]);
		if (cond_rr.err != NULL) {
			return cond_rr;
		}

		if (cond_rr.node->type == AST_QUOTED) {
			continue;
		}
		assert(cond_rr.node->type == AST_NUM);

		if (cond_rr.node->num.val) {
			return builtin_do(
				scope,
				"do",
				expr->len-1,
				(const Node **)expr->nodes+1
			);
		}
	}

	for (size_t i = 0; i < nargs; i++) {
		EXPECT_TYPE(i, AST_EXPR);

		const Expression *expr = &args[i]->expr;
		const Node *condition = expr->nodes[0];

		if (isQuoted(condition, "else")) {
			return builtin_do(
				scope,
				"do",
				expr->len-1,
				(const Node **)expr->nodes+1
			);
		}
	}

	return rr_null();
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

static Builtin makeBuiltin(const char *name, RunResult (*fn)(Scope*, const char*, size_t, const Node**)) {
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

BuiltinList *builtins_make(bool addPrelude) {
	BuiltinList *res = malloc(1, sizeof(BuiltinList));

	res->len = 0;
	res->cap = 1;
	res->items = calloc(res->cap, sizeof(Builtin));

	if (!addPrelude) {
		return res;
	}

	addBuiltin(res, "print", builtin_print);
	addBuiltin(res, "+", builtin_arith);
	addBuiltin(res, "-", builtin_arith);
	addBuiltin(res, "/", builtin_arith);
	addBuiltin(res, "*", builtin_arith);
	addBuiltin(res, "^", builtin_arith);
	addBuiltin(res, "==", builtin_comp);
	addBuiltin(res, "!=", builtin_comp);
	addBuiltin(res, "<", builtin_comp);
	addBuiltin(res, ">", builtin_comp);
	addBuiltin(res, "<=", builtin_comp);
	addBuiltin(res, ">=", builtin_comp);
	addBuiltin(res, "do", builtin_do);
	addBuiltin(res, "if", builtin_if);
	addBuiltin(res, "set", builtin_set);
	addBuiltin(res, "let", builtin_let);
	addBuiltin(res, "streq", builtin_streq);
	addBuiltin(res, "fun", builtin_fun);
	addBuiltin(res, "concat", builtin_concat);
	addBuiltin(res, "times", builtin_times);
	addBuiltin(res, "eval", builtin_eval);
	addBuiltin(res, "assert", builtin_assert);
	addBuiltin(res, "cond", builtin_cond);
	addBuiltin(res, "to-number", builtin_to_number);
	addBuiltin(res, "input", builtin_input);

	// list stuff
	addBuiltin(res, "list", builtin_list);
	addBuiltin(res, "car", builtin_car);
	addBuiltin(res, "cdr", builtin_cdr);

	return res;
}

BuiltinList *builtins_copy(const BuiltinList *builtins) {
	if (builtins == NULL) {
		return NULL;
	}

	BuiltinList *res = malloc(1, sizeof(BuiltinList));

	res->cap = builtins->cap;
	printf("%zu\n", res->cap);
	res->len = builtins->len;

	res->items = calloc(res->cap, sizeof(Builtin));
	memcpy(res->items, builtins->items, res->cap*sizeof(Builtin));

	return res;
}

void builtins_free(BuiltinList *builtins) {
	if (builtins == NULL) {
		return;
	}

	free(builtins->items);
	free(builtins);
}

void addBuiltin(BuiltinList* builtins, const char *name, BuiltinFn fn) {
	builtins->len++;
	if (builtins->cap < builtins->len) {
		builtins->cap *= 2;
		builtins->items = realloc(builtins->items, builtins->cap, sizeof(Builtin));
	}
	builtins->items[builtins->len - 1] = makeBuiltin(name, fn);
}

Builtin *getBuiltin(BuiltinList *builtins, const char *name) {
	for (size_t i = 0; i < builtins->len; i++) {
		Builtin *builtin = builtins->items + i;
		if (builtin->enabled && streq(name, builtin->name)) {
			return builtin;
		}
	}

	return NULL;
}

void enableBuiltin(BuiltinList *builtins, const char *name, bool enable) {
	getBuiltin(builtins, name)->enabled = enable;
}
