#include <strings.h>
#include <assert.h>
#include "../../ast.h"
#include "../../util.h"
#include "../interpreter.h"
#include "../../stringify.h"
#include "math.h"

RunResult builtin_arith(Scope *scope, const char *name, size_t nargs, const Node **args) {
#define CHECKNUM(node) do { \
	if (node == NULL || node->type != AST_NUM) { \
		return rr_errf("all arguments should be a number"); \
	} \
} while(0)

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
		case '%':
			res->num.val = fmod(res->num.val, n);
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

void init_builtins_math(BuiltinList *ls) {
	addBuiltin(ls, "+", builtin_arith);
	addBuiltin(ls, "-", builtin_arith);
	addBuiltin(ls, "/", builtin_arith);
	addBuiltin(ls, "*", builtin_arith);
	addBuiltin(ls, "^", builtin_arith);
	addBuiltin(ls, "%", builtin_arith);

	addBuiltin(ls, "==", builtin_comp);
	addBuiltin(ls, "!=", builtin_comp);
	addBuiltin(ls, "<", builtin_comp);
	addBuiltin(ls, ">", builtin_comp);
	addBuiltin(ls, "<=", builtin_comp);
	addBuiltin(ls, ">=", builtin_comp);
}
