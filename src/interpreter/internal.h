#pragma once

#include "../ast.h"
#include "./scope.h"

// TODO: better way for range
#define EXPECT(cond, n) do { \
	if (!(nargs cond n)) { \
		return rr_errf("expected number of args to %s %d but is %d", #cond, n, nargs); \
	} \
} while(0)

#define EXPECT_TYPE(narg, typ) do { \
	if (args[(narg)]->type != (typ)) { \
		return rr_errf("expected argument %d to be of type %s (but is of type %s)", narg, #typ, typetostr(args[(narg)])); \
	} \
} while(0)

typedef struct RunResult {
	// If err != NULL, node will be NULL.
	// Not-NULL values need to be freed.
	Node *node;
	char *err;
} RunResult;

RunResult rr_null(void);
RunResult rr_errf(const char*, ...);
RunResult rr_node(Node*);

Node *getVar(const Scope*, const char*);

RunResult run(Scope*, const Node*);

double getNumVal(Scope*, const Node*);

// ugly, should be removed later
RunResult runProgram(char *input, Scope *scope, bool intern);
